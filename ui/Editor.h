#ifndef FLORARPC_EDITOR_H
#define FLORARPC_EDITOR_H

#include <KSyntaxHighlighting/repository.h>
#include <KSyntaxHighlighting/syntaxhighlighter.h>
#include <google/protobuf/descriptor.h>
#include <grpc++/support/string_ref.h>

#include <QWidget>
#include <optional>

#include "../entity/Certificate.h"
#include "../entity/Method.h"
#include "../entity/Server.h"
#include "../entity/Session.h"
#include "florarpc/workspace.pb.h"
#include "ui/ui_Editor.h"

class Editor : public QWidget {
    Q_OBJECT

public:
    Editor(std::unique_ptr<Method> &&method, QWidget *parent = nullptr);

    inline Method &getMethod() { return *method; }

    void setServers(std::vector<std::shared_ptr<Server>> servers);

    void setCertificates(std::vector<std::shared_ptr<Certificate>> certificates);

    void readRequest(const florarpc::Request &request);

    void writeRequest(florarpc::Request &request);

    QString getRequestBody();

    std::optional<QHash<QString, QString>> getMetadata();

    std::shared_ptr<Server> getCurrentServer();

private slots:

    void onSendButtonClicked();

    void onFinishButtonClicked();

    void onCancelButtonClicked();

    void onResponseBodyPageChanged(int page);

    void onPrevResponseBodyButtonClicked();

    void onNextResponseBodyButtonClicked();

    void onMessageSent();

    void onMetadataReceived(const Session::Metadata &metadata);

    void onMessageReceived(const grpc::ByteBuffer &buffer);

    void onSessionFinished(int code, const QString &message, const QByteArray &details);

    void cleanupSession();

    void willEmitWorkspaceModified();

private:
    Ui_Editor ui;
    QMenu *responseMetadataContextMenu;
    Session *session;
    bool sendingRequest;
    QVector<grpc::ByteBuffer> responses;

    std::unique_ptr<Method> method;
    std::vector<std::shared_ptr<Server>> servers;
    std::vector<std::shared_ptr<Certificate>> certificates;

    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> requestHighlighter;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> requestMetadataHighlighter;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> responseHighlighter;

    void addMetadataRow(const QString &key, const QString &value);

    void clearResponseView();

    void setErrorToResponseView(const QString &code, const QString &message, const QString &details);

    void updateServerSelectBox();

    void updateSendButton();

    void enableStreamingButtons();

    void disableStreamingButtons();

    void updateResponsePager();
};

#endif  // FLORARPC_EDITOR_H
