#include "Editor.h"
#include "../util/GrpcUtility.h"
#include "../entity/Method.h"
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>
#include <QMenu>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>

Editor::Editor(std::unique_ptr<Method> &&method,
               KSyntaxHighlighting::Repository &repository,
               QWidget *parent)
        : QWidget(parent), method(std::move(method)), responseMetadataContextMenu(new QMenu(this)), session(nullptr) {
    ui.setupUi(this);

    connect(ui.executeButton, &QPushButton::clicked, this, &Editor::onExecuteButtonClicked);
    connect(ui.cancelButton, &QPushButton::clicked, this, &Editor::onCancelButtonClicked);

    const auto fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui.requestEdit->setFont(fixedFont);
    ui.requestMetadataEdit->setFont(fixedFont);
    ui.responseEdit->setFont(fixedFont);
    ui.errorDetailsEdit->setFont(fixedFont);

    const auto jsonDefinition = repository.definitionForMimeType("application/json");
    if (jsonDefinition.isValid()) {
        const auto theme = (palette().color(QPalette::Base).lightness() < 128) ?
                repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme) :
                repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme);
        requestHighlighter = setupHighlighter(*ui.requestEdit, jsonDefinition, theme);
        requestMetadataHighlighter = setupHighlighter(*ui.requestMetadataEdit, jsonDefinition, theme);
        responseHighlighter = setupHighlighter(*ui.responseEdit, jsonDefinition, theme);
    }

    ui.responseTabs->removeTab(ui.responseTabs->indexOf(ui.responseErrorTab));

    QStringList metadataHeaderLabels;
    metadataHeaderLabels.append("Key");
    metadataHeaderLabels.append("Value");
    ui.responseMetadataTable->setHorizontalHeaderLabels(metadataHeaderLabels);
    ui.responseMetadataTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
    ui.responseMetadataTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);
    ui.responseMetadataTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.responseMetadataTable, &QWidget::customContextMenuRequested, [=](const QPoint &pos) {
        if (!ui.responseMetadataTable->selectedItems().isEmpty()) {
            responseMetadataContextMenu->exec(ui.responseMetadataTable->viewport()->mapToGlobal(pos));
        }
    });
    responseMetadataContextMenu->addAction("コピー(&C)", [=](){
        auto selected = ui.responseMetadataTable->selectedItems();
        if (!selected.isEmpty()) {
            QApplication::clipboard()->setText(selected.first()->text());
        }
    })->setIcon(QIcon::fromTheme("edit-copy"));

    ui.requestEdit->setText(QString::fromStdString(this->method->makeRequestSkeleton()));
    requestHighlighter->rehighlight();

    hideStreamingButtons();
}

void Editor::onExecuteButtonClicked() {
    if (session != nullptr) {
        return;
    }

    clearResponseView();

    // Parse request body
    google::protobuf::DynamicMessageFactory dmf;
    std::unique_ptr<google::protobuf::Message> reqMessage;
    try {
        reqMessage = method->parseRequest(dmf, ui.requestEdit->toPlainText().toStdString());
    } catch (Method::ParseError &e) {
        setErrorToResponseView("-", "Request Parse Error", QString::fromStdString(e.getMessage()));
        return;
    }
    std::unique_ptr<grpc::ByteBuffer> sendBuffer = GrpcUtility::serializeMessage(*reqMessage);

    // Parse request metadata
    Session::Metadata metadata;
    if (const auto metadataInput = ui.requestMetadataEdit->toPlainText(); !metadataInput.isEmpty()) {
        QJsonParseError parseError = {};
        QJsonDocument metadataJson = QJsonDocument::fromJson(metadataInput.toUtf8(), &parseError);
        if (metadataJson.isNull()) {
            setErrorToResponseView("-", "Request Metadata Parse Error", parseError.errorString());
            return;
        }
        if (!metadataJson.isObject()) {
            setErrorToResponseView("-", "Request Metadata Parse Error", "Metadata input must be an object.");
            return;
        }

        const QJsonObject &object = metadataJson.object();
        for (auto iter = object.constBegin(); iter != object.constEnd(); iter++) {
            const auto key = iter.key();
            const auto value = iter.value().toString();
            if (value == nullptr) {
                setErrorToResponseView("-", "Request Metadata Parse Error",
                                       QLatin1String("Metadata '") + key + QLatin1String("' must be a string."));
                return;
            }

            if (key.endsWith("-bin")) {
                metadata.insert(key, QByteArray::fromBase64(value.toUtf8()));
            } else {
                metadata.insert(key, value);
            }
        }
    }

    session = new Session(*method, ui.serverAddressEdit->text(), metadata, this);
    connect(session, &Session::messageReceived, this, &Editor::onMessageReceived);
    connect(session, &Session::initialMetadataReceived, this, &Editor::onMetadataReceived);
    connect(session, &Session::trailingMetadataReceived, this, &Editor::onMetadataReceived);
    connect(session, &Session::finished, this, &Editor::onSessionFinished);
    connect(session, &Session::aborted, this, &Editor::cleanupSession);

    emit session->send(*sendBuffer);
    ui.executeButton->setDisabled(true);
    if (method->isClientStreaming() || method->isServerStreaming()) {
        showStreamingButtons();
    }
}

void Editor::onCancelButtonClicked() {
    if (session == nullptr) {
        return;
    }

    session->finish();
}

void Editor::onMessageReceived(const grpc::ByteBuffer &buffer) {
    google::protobuf::DynamicMessageFactory dmf;
    auto resMessage = method->parseResponse(dmf, buffer);
    std::string out;
    google::protobuf::util::JsonOptions opts;
    opts.add_whitespace = true;
    opts.always_print_primitive_fields = true;
    google::protobuf::util::MessageToJsonString(*resMessage, &out, opts);
    ui.responseEdit->setText(QString::fromStdString(out));
    responseHighlighter->rehighlight();

    ui.responseTabs->removeTab(ui.responseTabs->indexOf(ui.responseErrorTab));
    ui.responseTabs->insertTab(0, ui.responseBodyTab, "Body");
    ui.responseTabs->setCurrentIndex(0);

    if (!(method->isClientStreaming() || method->isServerStreaming())) {
        emit session->finish();
    }
}

void Editor::onMetadataReceived(const Session::Metadata &metadata) {
    for (auto iter = metadata.cbegin(); iter != metadata.cend(); iter++) {
        addMetadataRow(iter.key(), iter.value());
    }

    if (ui.responseMetadataTable->rowCount() > 0) {
        ui.responseTabs->setTabText(ui.responseTabs->indexOf(ui.responseMetadataTab),
                                    QString::asprintf("Metadata (%d)", ui.responseMetadataTable->rowCount()));
    }
}

void Editor::onSessionFinished(int code, const QString &message, const QByteArray &details) {
    if (code != grpc::StatusCode::OK) {
        setErrorToResponseView(GrpcUtility::errorCodeToString((grpc::StatusCode) code), message, details);
    }
    cleanupSession();
}

void Editor::cleanupSession() {
    ui.executeButton->setDisabled(false);
    hideStreamingButtons();
    delete session;
    session = nullptr;
}

std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> Editor::setupHighlighter(
        QTextEdit &edit, const KSyntaxHighlighting::Definition &definition, const KSyntaxHighlighting::Theme &theme) {
    auto highlighter = std::make_unique<KSyntaxHighlighting::SyntaxHighlighter>(&edit);

    auto pal = qApp->palette();
    if (theme.isValid()) {
        pal.setColor(QPalette::Base, theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
        pal.setColor(QPalette::Highlight, theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));
    }
    setPalette(pal);

    highlighter->setDefinition(definition);
    highlighter->setTheme(theme);
    highlighter->rehighlight();

    return highlighter;
}

void Editor::addMetadataRow(const QString &key, const QString &value) {
    int row = ui.responseMetadataTable->rowCount();
    ui.responseMetadataTable->insertRow(row);
    const QString &keyString = key;
    ui.responseMetadataTable->setItem(row, 0, new QTableWidgetItem(keyString));

    if (keyString.endsWith("-bin")) {
        QByteArray byteValue(value.toUtf8());
        ui.responseMetadataTable->setItem(row, 1, new QTableWidgetItem(QString::fromLatin1(byteValue.toBase64())));
    } else {
        ui.responseMetadataTable->setItem(row, 1, new QTableWidgetItem(value));
    }
}

void Editor::clearResponseView() {
    ui.responseEdit->clear();
    ui.responseMetadataTable->clearContents();
    ui.responseMetadataTable->setRowCount(0);
    ui.responseTabs->setTabText(ui.responseTabs->indexOf(ui.responseMetadataTab), "Metadata");
}

void Editor::setErrorToResponseView(const QString &code, const QString &message, const QString &details) {
    ui.errorCodeLabel->setText(code);
    ui.errorMessageLabel->setText(message);
    ui.errorDetailsEdit->setText(details);

    ui.responseTabs->removeTab(ui.responseTabs->indexOf(ui.responseBodyTab));
    ui.responseTabs->insertTab(0, ui.responseErrorTab, "Error");
    ui.responseTabs->setCurrentIndex(0);
}

void Editor::showStreamingButtons() {
    ui.executeButton->hide();
    ui.cancelButton->show();

    if (method->isClientStreaming()) {
        ui.sendButton->show();
        ui.finishButton->show();
    }
}

void Editor::hideStreamingButtons() {
    ui.executeButton->show();
    ui.sendButton->hide();
    ui.finishButton->hide();
    ui.cancelButton->hide();
}
