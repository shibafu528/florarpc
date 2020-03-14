#include "Editor.h"
#include "../util/GrpcUtility.h"
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>
#include <QMenu>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>

Editor::Editor(const google::protobuf::MethodDescriptor *descriptor,
               KSyntaxHighlighting::Repository &repository,
               QWidget *parent)
        : QWidget(parent), descriptor(descriptor), responseMetadataContextMenu(new QMenu(this)) {
    ui.setupUi(this);

    connect(ui.executeButton, &QPushButton::clicked, this, &Editor::onExecuteButtonClicked);

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

    google::protobuf::DynamicMessageFactory dmf;
    auto proto = dmf.GetPrototype(descriptor->input_type());
    auto message = std::unique_ptr<google::protobuf::Message>(proto->New());
    google::protobuf::util::JsonOptions opts;
    opts.add_whitespace = true;
    opts.always_print_primitive_fields = true;
    std::string out;
    google::protobuf::util::MessageToJsonString(*message, &out, opts);
    ui.requestEdit->setText(QString::fromStdString(out));
    requestHighlighter->rehighlight();
}

void Editor::onExecuteButtonClicked() {
    clearResponseView();

    // Parse request body
    google::protobuf::DynamicMessageFactory dmf;
    auto reqProto = dmf.GetPrototype(descriptor->input_type());
    auto reqMessage = std::unique_ptr<google::protobuf::Message>(reqProto->New());
    google::protobuf::util::JsonParseOptions parseOptions;
    parseOptions.ignore_unknown_fields = true;
    parseOptions.case_insensitive_enum_parsing = true;
    auto parseStatus = google::protobuf::util::JsonStringToMessage(ui.requestEdit->toPlainText().toStdString(), reqMessage.get(), parseOptions);
    if (!parseStatus.ok()) {
        setErrorToResponseView("-", "Request Parse Error", QString::fromStdString(parseStatus.error_message()));
        return;
    }

    // Parse request metadata
    grpc::ClientContext ctx;
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
                ctx.AddMetadata(key.toStdString(), QByteArray::fromBase64(value.toUtf8()).toStdString());
            } else {
                ctx.AddMetadata(key.toStdString(), value.toStdString());
            }
        }
    }

    std::unique_ptr<grpc::ByteBuffer> sendBuffer = GrpcUtility::serializeMessage(*reqMessage);
    auto ch = grpc::CreateChannel(ui.serverAddressEdit->text().toStdString(), grpc::InsecureChannelCredentials());
    grpc::GenericStub stub(ch);
    const std::string methodName = "/" + descriptor->service()->full_name() + "/" + descriptor->name();

    grpc_impl::CompletionQueue cq;
    auto call = stub.PrepareUnaryCall(&ctx, methodName, *sendBuffer, &cq);
    call->StartCall();
    grpc::ByteBuffer receiveBuffer;
    grpc::Status status;
    void *tag = (void*) 1;
    call->Finish(&receiveBuffer, &status, tag);

    void* gotTag;
    bool ok = false;
    cq.Next(&gotTag, &ok);
    if (ok && gotTag == tag) {
        if (status.ok()) {
            auto resProto = dmf.GetPrototype(descriptor->output_type());
            auto resMessage = std::unique_ptr<google::protobuf::Message>(resProto->New());
            GrpcUtility::parseMessage(receiveBuffer, *resMessage);
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
        } else {
            setErrorToResponseView(GrpcUtility::errorCodeToString(status.error_code()),
                                   QString::fromStdString(status.error_message()),
                                   QString::fromStdString(status.error_details()));
        }

        for (auto [key, value] : ctx.GetServerInitialMetadata()) {
            addMetadataRow(key, value);
        }
        for (auto [key, value] : ctx.GetServerTrailingMetadata()) {
            addMetadataRow(key, value);
        }
        if (ui.responseMetadataTable->rowCount() > 0) {
            ui.responseTabs->setTabText(ui.responseTabs->indexOf(ui.responseMetadataTab),
                                        QString::asprintf("Metadata (%d)", ui.responseMetadataTable->rowCount()));
        }
    }
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

void Editor::addMetadataRow(const grpc::string_ref &key, const grpc::string_ref &value) {
    int row = ui.responseMetadataTable->rowCount();
    ui.responseMetadataTable->insertRow(row);
    const QString &keyString = QString::fromLatin1(key.data(), key.size());
    ui.responseMetadataTable->setItem(row, 0, new QTableWidgetItem(keyString));

    if (keyString.endsWith("-bin")) {
        QByteArray byteValue(value.data(), value.size());
        ui.responseMetadataTable->setItem(row, 1, new QTableWidgetItem(QString::fromLatin1(byteValue.toBase64())));
    } else {
        ui.responseMetadataTable->setItem(row, 1, new QTableWidgetItem(QString::fromLatin1(value.data(), value.size())));
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
