#ifndef FLORARPC_EDITOR_H
#define FLORARPC_EDITOR_H

#include "../entity/Method.h"
#include "../entity/Session.h"
#include <QWidget>
#include <google/protobuf/descriptor.h>
#include <KSyntaxHighlighting/syntaxhighlighter.h>
#include <KSyntaxHighlighting/repository.h>
#include <grpc++/support/string_ref.h>
#include "ui/ui_Editor.h"

class Editor : public QWidget {
    Q_OBJECT

public:
    Editor(std::unique_ptr<Method> &&method,
           KSyntaxHighlighting::Repository &repository,
           QWidget *parent = nullptr);
    inline Method& getMethod() { return *method; }

private slots:
    void onServerAddressEditTextChanged(const QString &text);
    void onExecuteButtonClicked();
    void onSendButtonClicked();
    void onFinishButtonClicked();
    void onCancelButtonClicked();
    void onResponseBodyPageChanged(int page);
    void onPrevResponseBodyButtonClicked();
    void onNextResponseBodyButtonClicked();
    void onMetadataReceived(const Session::Metadata &metadata);
    void onMessageReceived(const grpc::ByteBuffer &buffer);
    void onSessionFinished(int code, const QString &message, const QByteArray &details);
    void cleanupSession();

private:
    Ui_Editor ui;
    QMenu *responseMetadataContextMenu;
    Session *session;
    QVector<grpc::ByteBuffer> responses;

    std::unique_ptr<Method> method;

    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> requestHighlighter;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> requestMetadataHighlighter;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> responseHighlighter;

    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> setupHighlighter(
            QTextEdit &edit, const KSyntaxHighlighting::Definition &definition, const KSyntaxHighlighting::Theme &theme);
    void addMetadataRow(const QString &key, const QString &value);
    void clearResponseView();
    void setErrorToResponseView(const QString &code, const QString &message, const QString &details);
    void showStreamingButtons();
    void hideStreamingButtons();
    void updateResponsePager();
};


#endif //FLORARPC_EDITOR_H
