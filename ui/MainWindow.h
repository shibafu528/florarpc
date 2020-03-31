#ifndef FLORARPC_MAINWINDOW_H
#define FLORARPC_MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>
#include <KSyntaxHighlighting/repository.h>
#include "ui/ui_MainWindow.h"
#include "../entity/Protocol.h"
#include "ProtocolTreeModel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onActionOpenTriggered();
    void onActionManageProtoTriggered();
    void onTreeViewClicked(const QModelIndex &index);
    void onEditorTabCloseRequested(const int index);
    void onTabCloseShortcutActivated();

private:
    Ui::MainWindow ui;
    std::vector<std::shared_ptr<Protocol>> protocols;
    std::unique_ptr<ProtocolTreeModel> protocolTreeModel;
    QStringList imports;
    KSyntaxHighlighting::Repository syntaxDefinitions;
    QShortcut tabCloseShortcut;
    QMenu treeMethodContextMenu;

    void openMethod(const QModelIndex &index, bool forceNewTab);
};


#endif //FLORARPC_MAINWINDOW_H
