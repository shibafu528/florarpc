#ifndef FLORARPC_SERVERSMANAGEDIALOG_H
#define FLORARPC_SERVERSMANAGEDIALOG_H

#include <QDialog>

#include "entity/Certificate.h"
#include "entity/Server.h"
#include "ui/ui_ServersManageDialog.h"

class ServersManageDialog : public QDialog {
    Q_OBJECT

public:
    explicit ServersManageDialog(QWidget *parent = nullptr);

    void setServers(std::vector<std::shared_ptr<Server>> &servers);

    std::vector<std::shared_ptr<Server>> &getServers();

private slots:
    void onAddServerButtonClick();

    void onEditServerButtonClick();

    void onDeleteServerButtonClick();

    void onAddCertsButtonClick();

    void onEditCertsButtonClick();

    void onDeleteCertsButtonClick();

    void onCloseButtonClick();

    void onServerCellDoubleClick(int row, int column);

private:
    Ui::ServersManageDialog ui;
    std::vector<std::shared_ptr<Server>> servers;

    void addServerRow(Server &server);

    void setServerRow(int row, Server &server);
};

#endif  // FLORARPC_SERVERSMANAGEDIALOG_H
