#include "MainWindow.h"
#include "../entity/Protocol.h"
#include <QStyle>
#include <QScreen>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <memory>
#include <google/protobuf/dynamic_message.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>
#include <google/protobuf/util/json_util.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui.setupUi(this);

    connect(ui.openProtoButton, &QPushButton::clicked, this, &MainWindow::onOpenProtoButtonClicked);
    connect(ui.treeView, &QTreeView::clicked, this, &MainWindow::onTreeViewClicked);
    connect(ui.executeButton, &QPushButton::clicked, this, &MainWindow::onExecuteButtonClicked);

    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(),
            QGuiApplication::primaryScreen()->availableGeometry()));
}

void MainWindow::onOpenProtoButtonClicked() {
    auto filename = QFileDialog::getOpenFileName(this, "Open proto", "",
            "Proto definition files (*.proto)", nullptr);
    if (filename.isEmpty()) {
        return;
    }
    QFileInfo file(filename);
    try {
        currentProtocol = std::move(Protocol::loadFromFile(file));
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

void MainWindow::onTreeViewClicked(const QModelIndex &index) {
    if (!index.parent().isValid() || !index.flags().testFlag(Qt::ItemFlag::ItemIsEnabled)) {
        // is not method node or disabled
        return;
    }

    auto method = static_cast<MethodNode*>(index.internalPointer());
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

static std::unique_ptr<grpc::ByteBuffer> serialize_message(const google::protobuf::Message &message) {
    std::string buffer;
    message.SerializeToString(&buffer);
    auto slice = grpc::Slice(buffer);
    return std::make_unique<grpc::ByteBuffer>(&slice, 1);
}

static bool parse_message(grpc::ByteBuffer &buffer, google::protobuf::Message &message) {
    std::vector<grpc::Slice> slices;
    buffer.Dump(&slices);
    std::string buf;
    buf.reserve(buffer.Length());
    for (const auto &s : slices) {
        buf.append(reinterpret_cast<const char*>(s.begin()), s.size());
    }
    return message.ParseFromString(buf);
}

static QString error_code_to_string(grpc::StatusCode statusCode) {
    QString code;
    switch (statusCode) {
        case grpc::OK:
            code = "OK";
            break;
        case grpc::CANCELLED:
            code = "CANCELLED";
            break;
        case grpc::UNKNOWN:
            code = "UNKNOWN";
            break;
        case grpc::INVALID_ARGUMENT:
            code = "INVALID_ARGUMENT";
            break;
        case grpc::DEADLINE_EXCEEDED:
            code = "DEADLINE_EXCEEDED";
            break;
        case grpc::NOT_FOUND:
            code = "NOT_FOUND";
            break;
        case grpc::ALREADY_EXISTS:
            code = "ALREADY_EXISTS";
            break;
        case grpc::PERMISSION_DENIED:
            code = "PERMISSION_DENIED";
            break;
        case grpc::UNAUTHENTICATED:
            code = "UNAUTHENTICATED";
            break;
        case grpc::RESOURCE_EXHAUSTED:
            code = "RESOURCE_EXHAUSTED";
            break;
        case grpc::FAILED_PRECONDITION:
            code = "FAILED_PRECONDITION";
            break;
        case grpc::ABORTED:
            code = "ABORTED";
            break;
        case grpc::OUT_OF_RANGE:
            code = "OUT_OF_RANGE";
            break;
        case grpc::UNIMPLEMENTED:
            code = "UNIMPLEMENTED";
            break;
        case grpc::INTERNAL:
            code = "INTERNAL";
            break;
        case grpc::UNAVAILABLE:
            code = "UNAVAILABLE";
            break;
        case grpc::DATA_LOSS:
            code = "DATA_LOSS";
            break;
        default:
            code = "?";
            break;
    }
    code += QString::asprintf(" (%d)", statusCode);
    return code;
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
        QString result;
        QTextStream stream(&result);
        stream << "[Request Parse Error]\n" << QString::fromStdString(parseStatus.ToString());
        ui.responseEdit->setText(result);
        return;
    }

    std::unique_ptr<grpc::ByteBuffer> sendBuffer = serialize_message(*reqMessage);
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
            parse_message(receiveBuffer, *resMessage);
            std::string out;
            google::protobuf::util::JsonOptions opts;
            opts.add_whitespace = true;
            opts.always_print_primitive_fields = true;
            google::protobuf::util::MessageToJsonString(*resMessage, &out, opts);
            ui.responseEdit->setText(QString::fromStdString(out));
        } else {
            QString result;
            QTextStream stream(&result);

            stream << "[gRPC Error]\n - Error Code: " << error_code_to_string(status.error_code()) <<
                "\n - Error Message: " << QString::fromStdString(status.error_message()) <<
                "\n - Details:\n" << QString::fromStdString(status.error_details());
            ui.responseEdit->setText(result);
        }
    }
}
