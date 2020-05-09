#include "ServersManageDialog.h"

#include <QMessageBox>

#include "ServerEditDialog.h"

ServersManageDialog::ServersManageDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint) {
    ui.setupUi(this);

    connect(ui.serversTable, &QTableWidget::cellDoubleClicked, this, &ServersManageDialog::onServerCellDoubleClick);
    connect(ui.addServerButton, &QAbstractButton::clicked, this, &ServersManageDialog::onAddServerButtonClick);
    connect(ui.editServerButton, &QAbstractButton::clicked, this, &ServersManageDialog::onEditServerButtonClick);
    connect(ui.deleteServerButton, &QAbstractButton::clicked, this, &ServersManageDialog::onDeleteServerButtonClick);
    connect(ui.buttonBox->button(QDialogButtonBox::Close), &QAbstractButton::clicked, this,
            &ServersManageDialog::onCloseButtonClick);

    auto serversTableHeader = ui.serversTable->horizontalHeader();
    serversTableHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    serversTableHeader->setSectionResizeMode(1, QHeaderView::Stretch);
    serversTableHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

void ServersManageDialog::setServers(std::vector<std::shared_ptr<Server>> &servers) {
    this->servers = servers;

    ui.serversTable->clearContents();
    for (auto &server : servers) {
        addServerRow(*server);
    }
}

std::vector<std::shared_ptr<Server>> &ServersManageDialog::getServers() { return servers; }

void ServersManageDialog::onAddServerButtonClick() {
    auto server = std::make_shared<Server>();
    auto dialog = std::make_unique<ServerEditDialog>(server, this);
    if (dialog->exec() == QDialog::DialogCode::Accepted) {
        servers.push_back(server);
        addServerRow(*server);
    }
}

void ServersManageDialog::onEditServerButtonClick() {
    auto select = ui.serversTable->selectionModel();
    if (!select->hasSelection()) {
        return;
    }

    auto row = select->selectedRows().first();
    auto server = servers[row.row()];
    auto dialog = std::make_unique<ServerEditDialog>(server, this);
    if (dialog->exec() == QDialog::DialogCode::Accepted) {
        setServerRow(row.row(), *server);
    }
}

void ServersManageDialog::onDeleteServerButtonClick() {
    auto select = ui.serversTable->selectionModel();
    if (!select->hasSelection()) {
        return;
    }

    auto row = select->selectedRows().first().row();
    if (QMessageBox::warning(this, "確認", QString("接続先 %1 を削除してもよろしいですか？").arg(servers[row]->name),
                             QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok) {
        return;
    }

    ui.serversTable->removeRow(row);
    servers.erase(servers.begin() + row);
}

void ServersManageDialog::onCloseButtonClick() { close(); }

void ServersManageDialog::onServerCellDoubleClick(int row, int column) {
    auto server = servers[row];
    auto dialog = std::make_unique<ServerEditDialog>(server, this);
    if (dialog->exec() == QDialog::DialogCode::Accepted) {
        setServerRow(row, *server);
    }
}

void ServersManageDialog::addServerRow(Server &server) {
    int row = ui.serversTable->rowCount();
    ui.serversTable->insertRow(row);
    setServerRow(row, server);
}

void ServersManageDialog::setServerRow(int row, Server &server) {
    ui.serversTable->setItem(row, 0, new QTableWidgetItem(server.name));
    ui.serversTable->setItem(row, 1, new QTableWidgetItem(server.address));

    auto useTLSItem = new QTableWidgetItem();
    useTLSItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    useTLSItem->setCheckState(server.useTLS ? Qt::Checked : Qt::Unchecked);
    ui.serversTable->setItem(row, 2, useTLSItem);
}
