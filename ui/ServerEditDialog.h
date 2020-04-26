#ifndef FLORARPC_SERVEREDITDIALOG_H
#define FLORARPC_SERVEREDITDIALOG_H

#include <QDialog>

#include "entity/Server.h"
#include "ui/ui_ServerEditDialog.h"

class ServerEditDialog : public QDialog {
    Q_OBJECT

public:
    ServerEditDialog(std::shared_ptr<Server> server, QWidget *parent = nullptr);

private slots:
    void onOkButtonClick();

    void onCancelButtonClick();

private:
    Ui::ServerEditDialog ui;
    std::shared_ptr<Server> server;
};

#endif  // FLORARPC_SERVEREDITDIALOG_H
