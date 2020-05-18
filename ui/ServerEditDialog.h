#ifndef FLORARPC_SERVEREDITDIALOG_H
#define FLORARPC_SERVEREDITDIALOG_H

#include <QDialog>

#include "entity/Certificate.h"
#include "entity/Server.h"
#include "ui/ui_ServerEditDialog.h"

class ServerEditDialog : public QDialog {
    Q_OBJECT

public:
    ServerEditDialog(std::shared_ptr<Server> server, std::vector<std::shared_ptr<Certificate>> certificates,
                     QWidget *parent = nullptr);

private slots:
    void onUseTLSCheckChanged(int state);

    void onOkButtonClick();

    void onCancelButtonClick();

private:
    Ui::ServerEditDialog ui;
    std::shared_ptr<Server> server;
    std::vector<std::shared_ptr<Certificate>> certificates;
};

#endif  // FLORARPC_SERVEREDITDIALOG_H
