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

    void setCertificates(std::vector<std::shared_ptr<Certificate>> &certificates);

    std::vector<std::shared_ptr<Certificate>> &getCertificates();

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
    std::vector<std::shared_ptr<Certificate>> certificates;

    void addServerRow(Server &server);

    void setServerRow(int row, Server &server);

    void addCertsRow(Certificate &certificate);

    void setCertsRow(int row, Certificate &certificate);
};

#endif  // FLORARPC_SERVERSMANAGEDIALOG_H
