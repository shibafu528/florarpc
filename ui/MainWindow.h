#ifndef FLORARPC_MAINWINDOW_H
#define FLORARPC_MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
#include <SyntaxHighlighter>
#include <Repository>
#include <Definition>
#include <grpcpp/grpcpp.h>
#include "ui/ui_MainWindow.h"
#include "../entity/Protocol.h"
#include "ProtocolTreeModel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onActionOpenTriggered();
    void onActionManageProtoTriggered();
    void onTreeViewClicked(const QModelIndex &index);
    void onExecuteButtonClicked();

private:
    Ui::MainWindow ui;
    std::vector<std::shared_ptr<Protocol>> protocols;
    std::unique_ptr<ProtocolTreeModel> protocolTreeModel;
    google::protobuf::MethodDescriptor const *currentMethod = nullptr;
    QStringList imports;
    QMenu *responseMetadataContextMenu;
    KSyntaxHighlighting::Repository syntaxDefinitions;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> requestHighlighter;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> requestMetadataHighlighter;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> responseHighlighter;

    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> setupHighlighter(QTextEdit &edit,
            const KSyntaxHighlighting::Definition &definition, const KSyntaxHighlighting::Theme &theme);
    void addMetadataRow(const grpc::string_ref &key, const grpc::string_ref &value);
};


#endif //FLORARPC_MAINWINDOW_H
