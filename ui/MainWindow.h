#ifndef FLORARPC_MAINWINDOW_H
#define FLORARPC_MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
#include "ui/ui_MainWindow.h"
#include "../entity/Protocol.h"
#include "ProtocolModel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onOpenProtoButtonClicked();
    void onTreeViewClicked(const QModelIndex &index);
    void onExecuteButtonClicked();

private:
    Ui::MainWindow ui;
    std::unique_ptr<Protocol> currentProtocol;
    MethodNode const *currentMethod = nullptr;
};


#endif //FLORARPC_MAINWINDOW_H
