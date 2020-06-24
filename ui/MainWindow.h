#ifndef FLORARPC_MAINWINDOW_H
#define FLORARPC_MAINWINDOW_H

#include <KSyntaxHighlighting/repository.h>

#include <QMainWindow>
#include <QShortcut>

#include "../entity/Certificate.h"
#include "../entity/Protocol.h"
#include "../entity/Server.h"
#include "Editor.h"
#include "ProtocolTreeModel.h"
#include "ui/ui_MainWindow.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

public slots:
    void onLogging(const QString &message);

private slots:

    void onActionOpenTriggered();

    void onActionOpenWorkspaceTriggered();

    void onActionSaveWorkspaceTriggered();

    void onActionManageProtoTriggered();

    void onActionManageServerTriggered();

    void onActionCopyAsGrpcurlTriggered();

    void onActionOpenCopyAsUserScriptDirTriggered();

    void onTreeViewClicked(const QModelIndex &index);

    void onRemoveFileFromTreeTriggered();

    void onEditorTabCloseRequested(const int index);

    void onTabCloseShortcutActivated();

private:
    Ui::MainWindow ui;
    std::vector<std::shared_ptr<Protocol>> protocols;
    std::vector<std::shared_ptr<Server>> servers;
    std::vector<std::shared_ptr<Certificate>> certificates;
    std::unique_ptr<ProtocolTreeModel> protocolTreeModel;
    QStringList imports;
    KSyntaxHighlighting::Repository syntaxDefinitions;
    QShortcut tabCloseShortcut;
    QMenu treeFileContextMenu;
    QMenu treeMethodContextMenu;
    QString workspaceFilename;

    void openProtos(const QStringList &filenames, bool abortOnLoadError);
    void openMethod(const QModelIndex &index, bool forceNewTab);
    Editor *openEditor(std::unique_ptr<Method> method, bool forceNewTab);
    bool saveWorkspace(const QString &filename);
    void setWorkspaceFilename(const QString &filename);
    void reloadCopyAsUserScripts();
    void executeCopyAsScript(const QString &script);
};

#endif  // FLORARPC_MAINWINDOW_H
