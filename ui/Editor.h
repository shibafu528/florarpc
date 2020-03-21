#ifndef FLORARPC_EDITOR_H
#define FLORARPC_EDITOR_H

#include "../entity/Method.h"
#include <QWidget>
#include <google/protobuf/descriptor.h>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Repository>
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
    void onExecuteButtonClicked();

private:
    Ui_Editor ui;
    QMenu *responseMetadataContextMenu;

    std::unique_ptr<Method> method;

    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> requestHighlighter;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> requestMetadataHighlighter;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> responseHighlighter;

    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> setupHighlighter(
            QTextEdit &edit, const KSyntaxHighlighting::Definition &definition, const KSyntaxHighlighting::Theme &theme);
    void addMetadataRow(const grpc::string_ref &key, const grpc::string_ref &value);
    void clearResponseView();
    void setErrorToResponseView(const QString &code, const QString &message, const QString &details);
    void showStreamingButtons();
    void hideStreamingButtons();
};


#endif //FLORARPC_EDITOR_H
