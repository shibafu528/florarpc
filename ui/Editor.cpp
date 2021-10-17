#include "Editor.h"

#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include <grpcpp/generic/generic_stub.h>
#include <grpcpp/grpcpp.h>

#include <QClipboard>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>

#include "../entity/Metadata.h"
#include "../entity/Method.h"
#include "../util/GrpcUtility.h"
#include "event/WorkspaceModifiedEvent.h"
#include "google/rpc/status.pb.h"
#include "util/SyntaxHighlighter.h"

static std::shared_ptr<grpc::ChannelCredentials> getCredentials(
    Server &server, std::vector<std::shared_ptr<Certificate>> &certificates) {
    if (server.useTLS) {
        auto certificate = server.findCertificate(certificates);
        if (certificate) {
            qDebug() << "Using ssl channel credentials with user options";
            return certificate->getCredentials();
        } else {
            qDebug() << "Using default ssl channel credentials";
            return grpc::SslCredentials(grpc::SslCredentialsOptions());
        }
    } else {
        qDebug() << "Using insecure channel credentials";
        return grpc::InsecureChannelCredentials();
    }
}

static grpc::ChannelArguments getChannelArguments(Server &server) {
    grpc::ChannelArguments args;
    if (server.useTLS && !server.tlsTargetNameOverride.isEmpty()) {
        args.SetSslTargetNameOverride(server.tlsTargetNameOverride.toStdString());
    }
    return args;
}

Editor::Editor(std::unique_ptr<Method> &&method, QWidget *parent)
    : QWidget(parent),
      responseMetadataContextMenu(new QMenu(this)),
      session(nullptr),
      sendingRequest(false),
      method(std::move(method)) {
    ui.setupUi(this);

    connect(ui.sendButton, &QPushButton::clicked, this, &Editor::onSendButtonClicked);
    connect(ui.finishButton, &QPushButton::clicked, this, &Editor::onFinishButtonClicked);
    connect(ui.cancelButton, &QPushButton::clicked, this, &Editor::onCancelButtonClicked);
    connect(ui.responseBodyPageSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &Editor::onResponseBodyPageChanged);
    connect(ui.prevResponseBodyButton, &QPushButton::clicked, this, &Editor::onPrevResponseBodyButtonClicked);
    connect(ui.nextResponseBodyButton, &QPushButton::clicked, this, &Editor::onNextResponseBodyButtonClicked);
    connect(ui.lastResponseBodyButton, &QPushButton::clicked, this, &Editor::onLastResponseBodyButtonClicked);
    connect(ui.serverSelectBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &Editor::willEmitWorkspaceModified);
    connect(ui.requestEdit, &QTextEdit::textChanged, this, &Editor::willEmitWorkspaceModified);
    connect(ui.requestMetadataEdit, &MetadataEdit::changed, this, &Editor::willEmitWorkspaceModified);

    const auto executeShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), this);
    connect(executeShortcut, &QShortcut::activated, this, &Editor::onSendButtonClicked);

    // 1:1にする
    // https://stackoverflow.com/a/43835396
    ui.splitter->setSizes({INT_MAX, INT_MAX});

    const auto fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui.requestEdit->setFont(fixedFont);
    ui.responseEdit->setFont(fixedFont);
    ui.errorDetailsEdit->setFont(fixedFont);

    requestHighlighter = SyntaxHighlighter::setup(*ui.requestEdit, palette());
    responseHighlighter = SyntaxHighlighter::setup(*ui.responseEdit, palette());

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
    responseMetadataContextMenu
        ->addAction("コピー(&C)",
                    [=]() {
                        auto selected = ui.responseMetadataTable->selectedItems();
                        if (!selected.isEmpty()) {
                            QApplication::clipboard()->setText(selected.first()->text());
                        }
                    })
        ->setIcon(QIcon::fromTheme("edit-copy"));

    ui.requestEdit->setText(QString::fromStdString(this->method->makeRequestSkeleton()));
    if (requestHighlighter) {
        requestHighlighter->rehighlight();
    }

    if (this->method->isServerStreaming()) {
        ui.responseBodyPagerWrapper->show();
        ui.responseBodyPager->setDisabled(true);
    } else {
        ui.responseBodyPagerWrapper->hide();
    }

    updateSendButton();
    if (this->method->isClientStreaming()) {
        ui.finishButton->show();
    } else {
        ui.finishButton->hide();
    }
}

void Editor::setServers(std::vector<std::shared_ptr<Server>> servers) {
    QUuid selected = nullptr;
    if (!this->servers.empty()) {
        selected = this->servers[ui.serverSelectBox->currentIndex()]->id;
    }

    this->servers = servers;
    ui.serverSelectBox->clear();
    if (servers.empty()) {
        ui.serverSelectBox->addItem("ファイル→接続先の管理 から追加してください");
        updateServerSelectBox();
        updateSendButton();
    } else {
        updateServerSelectBox();
        updateSendButton();
        for (std::vector<std::shared_ptr<Server>>::size_type i = 0; i < servers.size(); i++) {
            ui.serverSelectBox->addItem(servers[i]->name);
            if (selected == servers[i]->id) {
                ui.serverSelectBox->setCurrentIndex(i);
            }
        }
    }
}

void Editor::setCertificates(std::vector<std::shared_ptr<Certificate>> certificates) {
    this->certificates = certificates;
}

void Editor::readRequest(const florarpc::Request &request) {
    ui.requestEdit->setPlainText(QString::fromStdString(request.body_draft()));
    ui.requestMetadataEdit->setString(QString::fromStdString(request.metadata_draft()));
    for (std::vector<std::shared_ptr<Server>>::size_type i = 0; i < servers.size(); i++) {
        if (request.selected_server_id() == servers[i]->id.toString().toStdString()) {
            ui.serverSelectBox->setCurrentIndex(i);
            break;
        }
    }
    ui.useSharedMetadata->setChecked(request.use_shared_metadata());
}

void Editor::writeRequest(florarpc::Request &request) {
    florarpc::MethodRef *methodRef = request.mutable_method();
    method->writeMethodRef(*methodRef);

    request.set_body_draft(ui.requestEdit->toPlainText().toStdString());
    request.set_metadata_draft(ui.requestMetadataEdit->toString().toStdString());
    if (const auto server = getCurrentServer()) {
        request.set_selected_server_id(server->id.toByteArray().toStdString());
    } else {
        request.clear_selected_server_id();
    }
    request.set_use_shared_metadata(ui.useSharedMetadata->isChecked());
}

QString Editor::getRequestBody() { return ui.requestEdit->toPlainText(); }

std::optional<QHash<QString, QString>> Editor::getMetadata() {
    Metadata meta;
    if (auto server = getCurrentServer();
        server && !server->sharedMetadata.isEmpty() && ui.useSharedMetadata->isChecked()) {
        if (auto parseResult = meta.parseJson(server->sharedMetadata); !parseResult.isEmpty()) {
            QMessageBox::warning(this, "Shared Metadata Parse Error", parseResult);
            return std::nullopt;
        }
    }
    if (auto parseResult = meta.parseJson(ui.requestMetadataEdit->toString(), Metadata::MergeStrategy::Replace);
        !parseResult.isEmpty()) {
        QMessageBox::warning(this, "Request Metadata Parse Error", parseResult);
        return std::nullopt;
    }

    return meta.asHash();
}

void Editor::onSendButtonClicked() {
    const auto initialize = session == nullptr;

    if (initialize) {
        clearResponseView();
        responses.clear();
        ui.requestHistoryTab->clear();
        ui.responseBodyPageSpin->setValue(1);
        updateResponsePager();
        ui.responseBodyPager->setDisabled(true);
    }

    // Parse request body
    google::protobuf::DynamicMessageFactory dmf;
    std::unique_ptr<google::protobuf::Message> reqMessage;
    try {
        reqMessage = method->parseRequest(dmf, ui.requestEdit->toPlainText().toStdString());
    } catch (Method::ParseError &e) {
        if (initialize) {
            setErrorToResponseView("-", "Request Parse Error", QString::fromStdString(e.getMessage()));
        } else {
            // TODO: setErrorToResponseViewするとbodyが消えるので使わない、もっといい出し方考える
            QMessageBox::warning(this, "Request Parse Error", QString::fromStdString(e.getMessage()));
        }
        return;
    }
    std::unique_ptr<grpc::ByteBuffer> sendBuffer = GrpcUtility::serializeMessage(*reqMessage);

    if (initialize) {
        // Parse request metadata
        Metadata meta;
        if (auto server = getCurrentServer();
            server && !server->sharedMetadata.isEmpty() && ui.useSharedMetadata->isChecked()) {
            if (auto parseResult = meta.parseJson(server->sharedMetadata); !parseResult.isEmpty()) {
                setErrorToResponseView("-", "Shared Metadata Parse Error", parseResult);
                return;
            }
        }
        if (auto parseResult = meta.parseJson(ui.requestMetadataEdit->toString(), Metadata::MergeStrategy::Replace);
            !parseResult.isEmpty()) {
            setErrorToResponseView("-", "Request Metadata Parse Error", parseResult);
            return;
        }

        auto server = getCurrentServer();
        auto credentials = getCredentials(*server, certificates);
        auto channelArgs = getChannelArguments(*server);
        session = new Session(*method, server->address, credentials, channelArgs, meta.getValues(), this);
        connect(session, &Session::messageSent, this, &Editor::onMessageSent);
        connect(session, &Session::messageReceived, this, &Editor::onMessageReceived);
        connect(session, &Session::initialMetadataReceived, this, &Editor::onMetadataReceived);
        connect(session, &Session::trailingMetadataReceived, this, &Editor::onMetadataReceived);
        connect(session, &Session::finished, this, &Editor::onSessionFinished);
        connect(session, &Session::aborted, this, &Editor::cleanupSession);
    }

    emit session->send(*sendBuffer);

    sendingRequest = true;
    updateServerSelectBox();
    updateSendButton();
    updateCancelButton();
    if (method->isClientStreaming() || method->isServerStreaming()) {
        enableStreamingButtons();
    }

    ui.requestHistoryTab->append(ui.requestEdit->toPlainText());
}

void Editor::onFinishButtonClicked() {
    if (session == nullptr) {
        return;
    }

    emit session->done();

    updateSendButton();
    ui.finishButton->setDisabled(true);
}

void Editor::onCancelButtonClicked() {
    if (session == nullptr) {
        return;
    }

    emit session->cancel();

    updateSendButton();
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
    if (responseHighlighter) {
        responseHighlighter->rehighlight();
    }

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

void Editor::onLastResponseBodyButtonClicked() {
    ui.responseBodyPageSpin->setValue(responses.size());
    updateResponsePager();
}

void Editor::onMessageSent() {
    sendingRequest = false;
    updateSendButton();
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

    if (method->isServerStreaming() && ui.followResponseCheck->isChecked()) {
        auto current = ui.responseBodyPageSpin->value();
        if (current == responses.size() - 1) {
            ui.responseBodyPageSpin->setValue(responses.size());
        }
    }

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
        QString formattedDetails = details;
        if (!details.isEmpty()) {
            google::protobuf::DynamicMessageFactory dmf;
            const auto status = method->parseErrorDetails(dmf, details.toStdString());
            if (status) {
                std::string out;
                // TODO: JSONにしたい気持ちはあるけど、Anyの解決に失敗した時に何も出力されないのが困るから妥協した
                google::protobuf::TextFormat::Printer printer;
                printer.SetExpandAny(true);
                printer.SetUseUtf8StringEscaping(true);
                bool successPrint = printer.PrintToString(*status, &out);
                if (successPrint) {
                    formattedDetails = QString::fromStdString(out);
                }
            }
        }
        setErrorToResponseView(GrpcUtility::errorCodeToString((grpc::StatusCode)code), message, formattedDetails);
    }

    const auto elapsed = session->getEndTime() - session->getBeginTime();
    const auto secs = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    const auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() % 1000000;
    ui.responseElapsedLabel->setText(QString("%1.%2s").arg(secs).arg(nsecs));

    cleanupSession();
}

void Editor::cleanupSession() {
    delete session;
    session = nullptr;
    disableStreamingButtons();
    updateSendButton();
    updateCancelButton();
    updateServerSelectBox();
}

void Editor::willEmitWorkspaceModified() {
    QApplication::postEvent(window(), new Event::WorkspaceModifiedEvent(
                                          QString("%1:%2").arg(metaObject()->className()).arg(sender()->objectName())));
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
    ui.responseElapsedLabel->clear();
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

void Editor::updateServerSelectBox() { ui.serverSelectBox->setDisabled(session != nullptr || servers.empty()); }

void Editor::updateSendButton() {
    bool disabled = false;

    if (servers.empty()) {
        disabled = true;
    } else {
        if (session == nullptr) {
            disabled = false;
        } else {
            auto sequence = session->getSequence();
            if (sendingRequest || sequence == Session::Sequence::WritesDone ||
                sequence == Session::Sequence::Finishing) {
                disabled = true;
            } else {
                disabled = !method->isClientStreaming();
            }
        }
    }

    ui.sendButton->setDisabled(disabled);
}

void Editor::updateCancelButton() { ui.cancelButton->setDisabled(session == nullptr); }

void Editor::enableStreamingButtons() {
    if (method->isClientStreaming()) {
        ui.finishButton->setDisabled(false);
    }
}

void Editor::disableStreamingButtons() { ui.finishButton->setDisabled(true); }

void Editor::updateResponsePager() {
    ui.responseBodyPager->setDisabled(false);
    ui.responseBodyPageSpin->setMaximum(responses.isEmpty() ? 1 : responses.size());
    ui.responseBodyMaxPageLabel->setText(QString("%1").arg(responses.size()));

    if (responses.isEmpty()) {
        ui.prevResponseBodyButton->setDisabled(true);
        ui.nextResponseBodyButton->setDisabled(true);
        ui.lastResponseBodyButton->setDisabled(true);
    } else {
        ui.prevResponseBodyButton->setDisabled(false);
        ui.nextResponseBodyButton->setDisabled(false);
        ui.lastResponseBodyButton->setDisabled(false);
    }

    if (ui.responseBodyPageSpin->value() <= 1) {
        ui.prevResponseBodyButton->setDisabled(true);
    }
    if (ui.responseBodyPageSpin->value() == responses.size()) {
        ui.nextResponseBodyButton->setDisabled(true);
        ui.lastResponseBodyButton->setDisabled(true);
    }
}

std::shared_ptr<Server> Editor::getCurrentServer() {
    if (servers.empty()) {
        return nullptr;
    }

    return servers[ui.serverSelectBox->currentIndex()];
}
