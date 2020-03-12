#ifndef FLORARPC_MAINWINDOW_H
#define FLORARPC_MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
#include <SyntaxHighlighter>
#include <Repository>
#include <Definition>
#include "ui/ui_MainWindow.h"
#include "../entity/Protocol.h"
#include "ProtocolModel.h"

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
    std::unique_ptr<Protocol> currentProtocol;
    MethodNode const *currentMethod = nullptr;
    QStringList imports;
    KSyntaxHighlighting::Repository syntaxDefinitions;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> requestHighlighter;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> responseHighlighter;

    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> setupHighlighter(QTextEdit &edit,
            const KSyntaxHighlighting::Definition &definition, const KSyntaxHighlighting::Theme &theme);
};


#endif //FLORARPC_MAINWINDOW_H
