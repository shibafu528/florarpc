#include "Editor.h"
#include "../util/GrpcUtility.h"
#include "../entity/Method.h"
#include <KSyntaxHighlighting/definition.h>
#include <KSyntaxHighlighting/theme.h>
#include <QMenu>
#include <QMessageBox>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>

static std::shared_ptr<grpc::ChannelCredentials> getDefaultCredentials(bool useTls) {
    return useTls ? grpc::SslCredentials(grpc::SslCredentialsOptions()) : grpc::InsecureChannelCredentials();
}

Editor::Editor(std::unique_ptr<Method> &&method,
               KSyntaxHighlighting::Repository &repository,
               QWidget *parent)
        : QWidget(parent), responseMetadataContextMenu(new QMenu(this)), session(nullptr), method(std::move(method)) {
    ui.setupUi(this);

    connect(ui.serverAddressEdit, &QLineEdit::textChanged, this, &Editor::onServerAddressEditTextChanged);
    connect(ui.executeButton, &QPushButton::clicked, this, &Editor::onExecuteButtonClicked);
    connect(ui.sendButton, &QPushButton::clicked, this, &Editor::onSendButtonClicked);
    connect(ui.finishButton, &QPushButton::clicked, this, &Editor::onFinishButtonClicked);
    connect(ui.cancelButton, &QPushButton::clicked, this, &Editor::onCancelButtonClicked);
    connect(ui.responseBodyPageSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &Editor::onResponseBodyPageChanged);
    connect(ui.prevResponseBodyButton, &QPushButton::clicked, this, &Editor::onPrevResponseBodyButtonClicked);
    connect(ui.nextResponseBodyButton, &QPushButton::clicked, this, &Editor::onNextResponseBodyButtonClicked);

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

    ui.requestHistoryTab->setupHighlighter(repository);

    if (!this->method->isClientStreaming()) {
        ui.requestTabs->removeTab(ui.requestTabs->indexOf(ui.requestHistoryTab));
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

    if (this->method->isServerStreaming()) {
        ui.responseBodyPager->show();
        ui.responseBodyPager->setDisabled(true);
    } else {
        ui.responseBodyPager->hide();
    }

    hideStreamingButtons();
}

void Editor::onServerAddressEditTextChanged(const QString &text) {
    ui.executeButton->setEnabled(!text.isEmpty());
}

void Editor::onExecuteButtonClicked() {
    if (session != nullptr) {
        return;
    }

    clearResponseView();
    responses.clear();
    ui.requestHistoryTab->clear();
    ui.responseBodyPageSpin->setValue(1);
    updateResponsePager();
    ui.responseBodyPager->setDisabled(true);

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

    auto credentials = getDefaultCredentials(ui.useTlsCheck->isChecked());
    session = new Session(*method, ui.serverAddressEdit->text(), credentials, metadata, this);
    connect(session, &Session::messageSent, this, &Editor::onMessageSent);
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

    ui.requestHistoryTab->append(ui.requestEdit->toPlainText());
}

void Editor::onSendButtonClicked() {
    if (session == nullptr) {
        return;
    }

    // Parse request body
    google::protobuf::DynamicMessageFactory dmf;
    std::unique_ptr<google::protobuf::Message> reqMessage;
    try {
        reqMessage = method->parseRequest(dmf, ui.requestEdit->toPlainText().toStdString());
    } catch (Method::ParseError &e) {
        // TODO: setErrorToResponseViewするとbodyが消えるので使わない、もっといい出し方考える
        QMessageBox::warning(this, "Request Parse Error", QString::fromStdString(e.getMessage()));
        return;
    }
    std::unique_ptr<grpc::ByteBuffer> sendBuffer = GrpcUtility::serializeMessage(*reqMessage);
    emit session->send(*sendBuffer);

    ui.sendButton->setDisabled(true);
    ui.requestHistoryTab->append(ui.requestEdit->toPlainText());
}

void Editor::onFinishButtonClicked() {
    if (session == nullptr) {
        return;
    }

    emit session->done();

    ui.sendButton->setDisabled(true);
    ui.finishButton->setDisabled(true);
}

void Editor::onCancelButtonClicked() {
    if (session == nullptr) {
        return;
    }

    emit session->finish();

    ui.sendButton->setDisabled(true);
    ui.finishButton->setDisabled(true);
    ui.cancelButton->setDisabled(true);
}

void Editor::onResponseBodyPageChanged(int page) {
    if (responses.isEmpty() || page < 1 || page > responses.size()) {
        ui.responseEdit->clear();
        return;
    }

    google::protobuf::DynamicMessageFactory dmf;
    auto resMessage = method->parseResponse(dmf, responses[page - 1]);
    std::string out;
    google::protobuf::util::JsonOptions opts;
    opts.add_whitespace = true;
    opts.always_print_primitive_fields = true;
    google::protobuf::util::MessageToJsonString(*resMessage, &out, opts);
    ui.responseEdit->setText(QString::fromStdString(out));
    responseHighlighter->rehighlight();

    updateResponsePager();
}

void Editor::onPrevResponseBodyButtonClicked() {
    ui.responseBodyPageSpin->setValue(ui.responseBodyPageSpin->value() - 1);
    updateResponsePager();
}

void Editor::onNextResponseBodyButtonClicked() {
    ui.responseBodyPageSpin->setValue(ui.responseBodyPageSpin->value() + 1);
    updateResponsePager();
}

void Editor::onMessageSent() {
    ui.sendButton->setDisabled(false);
}

void Editor::onMessageReceived(const grpc::ByteBuffer &buffer) {
    responses.append(buffer);

    if (responses.size() == 1) {
        ui.responseTabs->removeTab(ui.responseTabs->indexOf(ui.responseErrorTab));
        ui.responseTabs->insertTab(0, ui.responseBodyTab, "Body");
        ui.responseTabs->setCurrentIndex(0);
        onResponseBodyPageChanged(1);
    }

    updateResponsePager();

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
    ui.cancelButton->setDisabled(false);

    if (method->isClientStreaming()) {
        ui.sendButton->show();
        ui.sendButton->setDisabled(true);
        ui.finishButton->show();
        ui.finishButton->setDisabled(false);
    }
}

void Editor::hideStreamingButtons() {
    ui.executeButton->show();
    ui.sendButton->hide();
    ui.finishButton->hide();
    ui.cancelButton->hide();
}

void Editor::updateResponsePager() {
    ui.responseBodyPager->setDisabled(false);
    ui.responseBodyPageSpin->setMaximum(responses.isEmpty() ? 1 : responses.size());
    ui.responseBodyMaxPageLabel->setText(QString("%1").arg(responses.size()));

    if (responses.isEmpty()) {
        ui.prevResponseBodyButton->setDisabled(true);
        ui.nextResponseBodyButton->setDisabled(true);
    } else {
        ui.prevResponseBodyButton->setDisabled(false);
        ui.nextResponseBodyButton->setDisabled(false);
    }

    if (ui.responseBodyPageSpin->value() <= 1) {
        ui.prevResponseBodyButton->setDisabled(true);
    }
    if (ui.responseBodyPageSpin->value() == responses.size()) {
        ui.nextResponseBodyButton->setDisabled(true);
    }
}
