#include "MainWindow.h"
#include "../entity/Protocol.h"
#include "../util/GrpcUtility.h"
#include "ImportsManageDialog.h"
#include <QStyle>
#include <QScreen>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFontDatabase>
#include <Theme>
#include <memory>
#include <google/protobuf/dynamic_message.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>
#include <google/protobuf/util/json_util.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui.setupUi(this);

    connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::onActionOpenTriggered);
    connect(ui.actionManageProto, &QAction::triggered, this, &MainWindow::onActionManageProtoTriggered);
    connect(ui.treeView, &QTreeView::clicked, this, &MainWindow::onTreeViewClicked);
    connect(ui.executeButton, &QPushButton::clicked, this, &MainWindow::onExecuteButtonClicked);

    const auto fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui.requestEdit->setFont(fixedFont);
    ui.responseEdit->setFont(fixedFont);
    ui.errorDetailsEdit->setFont(fixedFont);

    const auto jsonDefinition = syntaxDefinitions.definitionForMimeType("application/json");
    if (jsonDefinition.isValid()) {
        const auto theme = (palette().color(QPalette::Base).lightness() < 128) ? syntaxDefinitions.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme) : syntaxDefinitions.defaultTheme(KSyntaxHighlighting::Repository::LightTheme);
        requestHighlighter = setupHighlighter(*ui.requestEdit, jsonDefinition, theme);
        responseHighlighter = setupHighlighter(*ui.responseEdit, jsonDefinition, theme);
    }

    ui.responseTabs->removeTab(ui.responseTabs->indexOf(ui.responseErrorTab));

    QStringList metadataHeaderLabels;
    metadataHeaderLabels.append("Key");
    metadataHeaderLabels.append("Value");
    ui.responseMetadataTable->setHorizontalHeaderLabels(metadataHeaderLabels);
    ui.responseMetadataTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
    ui.responseMetadataTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);

    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(),
            QGuiApplication::primaryScreen()->availableGeometry()));
}

void MainWindow::onActionOpenTriggered() {
    auto filename = QFileDialog::getOpenFileName(this, "Open proto", "",
            "Proto definition files (*.proto)", nullptr);
    if (filename.isEmpty()) {
        return;
    }
    QFileInfo file(filename);
    try {
        currentProtocol = std::move(Protocol::loadFromFile(file, imports));
        ui.currentProtoLabel->setText(file.fileName());

        auto model = new ProtocolModel(ui.treeView, currentProtocol.get());
        ui.treeView->setModel(model);
        ui.treeView->expandAll();
    } catch (ProtocolLoadException &e) {
        QString message = "Protoファイルの読込中にエラーが発生しました。\n";
        QTextStream stream(&message);

        for (const auto& err : *e.errors) {
            if (&err != &e.errors->front()) {
                stream << '\n';
            }
            stream << QString::fromStdString(err);
        }
        QMessageBox::critical(this, "Load error", message);
    }
}

void MainWindow::onActionManageProtoTriggered() {
    auto dialog = std::make_unique<ImportsManageDialog>(this);
    dialog->setPaths(imports);
    dialog->exec();
    imports = dialog->getPaths();
}

void MainWindow::onTreeViewClicked(const QModelIndex &index) {
    if (!index.parent().isValid() || !index.flags().testFlag(Qt::ItemFlag::ItemIsEnabled)) {
        // is not method node or disabled
        return;
    }

    auto method = ProtocolModel::indexToMethod(index);
    currentMethod = method;
    ui.currentMethodLabel->setText(QString::fromStdString(method->descriptor->full_name()));
    ui.executeButton->setEnabled(true);

    google::protobuf::DynamicMessageFactory dmf;
    auto proto = dmf.GetPrototype(method->descriptor->input_type());
    auto message = std::unique_ptr<google::protobuf::Message>(proto->New());
    google::protobuf::util::JsonOptions opts;
    opts.add_whitespace = true;
    opts.always_print_primitive_fields = true;
    std::string out;
    google::protobuf::util::MessageToJsonString(*message, &out, opts);
    ui.requestEdit->setText(QString::fromStdString(out));
}

void MainWindow::onExecuteButtonClicked() {
    if (currentMethod == nullptr) {
        return;
    }

    google::protobuf::DynamicMessageFactory dmf;
    auto method = currentMethod->descriptor;

    auto reqProto = dmf.GetPrototype(method->input_type());
    auto reqMessage = std::unique_ptr<google::protobuf::Message>(reqProto->New());
    google::protobuf::util::JsonParseOptions parseOptions;
    parseOptions.ignore_unknown_fields = true;
    parseOptions.case_insensitive_enum_parsing = true;
    auto parseStatus = google::protobuf::util::JsonStringToMessage(ui.requestEdit->toPlainText().toStdString(), reqMessage.get(), parseOptions);
    if (!parseStatus.ok()) {
        ui.errorCodeLabel->setText("-");
        ui.errorMessageLabel->setText("Request Parse Error");
        ui.errorDetailsEdit->setText(QString::fromStdString(parseStatus.error_message()));

        ui.responseTabs->removeTab(ui.responseTabs->indexOf(ui.responseBodyTab));
        ui.responseTabs->insertTab(0, ui.responseErrorTab, "Error");
        ui.responseTabs->setCurrentIndex(0);
        return;
    }

    std::unique_ptr<grpc::ByteBuffer> sendBuffer = GrpcUtility::serializeMessage(*reqMessage);
    auto ch = grpc::CreateChannel(ui.serverAddressEdit->text().toStdString(), grpc::InsecureChannelCredentials());
    grpc::GenericStub stub(ch);
    grpc::ClientContext ctx;
    const std::string methodName = "/" + method->service()->full_name() + "/" + method->name();

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
            auto resProto = dmf.GetPrototype(method->output_type());
            auto resMessage = std::unique_ptr<google::protobuf::Message>(resProto->New());
            GrpcUtility::parseMessage(receiveBuffer, *resMessage);
            std::string out;
            google::protobuf::util::JsonOptions opts;
            opts.add_whitespace = true;
            opts.always_print_primitive_fields = true;
            google::protobuf::util::MessageToJsonString(*resMessage, &out, opts);
            ui.responseEdit->setText(QString::fromStdString(out));

            ui.responseTabs->removeTab(ui.responseTabs->indexOf(ui.responseErrorTab));
            ui.responseTabs->insertTab(0, ui.responseBodyTab, "Body");
            ui.responseTabs->setCurrentIndex(0);
        } else {
            ui.errorCodeLabel->setText(GrpcUtility::errorCodeToString(status.error_code()));
            ui.errorMessageLabel->setText(QString::fromStdString(status.error_message()));
            ui.errorDetailsEdit->setText(QString::fromStdString(status.error_details()));

            ui.responseTabs->removeTab(ui.responseTabs->indexOf(ui.responseBodyTab));
            ui.responseTabs->insertTab(0, ui.responseErrorTab, "Error");
            ui.responseTabs->setCurrentIndex(0);
        }

        ui.responseMetadataTable->clearContents();
        ui.responseMetadataTable->setRowCount(0);
        for (auto [key, value] : ctx.GetServerTrailingMetadata()) {
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
    }
}

std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> MainWindow::setupHighlighter(
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
