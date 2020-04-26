#include "ServersManageDialog.h"

#include <QMessageBox>

#include "ServerEditDialog.h"

ServersManageDialog::ServersManageDialog(QWidget *parent) : QDialog(parent) {
    ui.setupUi(this);

    connect(ui.addButton, &QAbstractButton::clicked, this, &ServersManageDialog::onAddButtonClick);
    connect(ui.editButton, &QAbstractButton::clicked, this, &ServersManageDialog::onEditButtonClick);
    connect(ui.deleteButton, &QAbstractButton::clicked, this, &ServersManageDialog::onDeleteButtonClick);
    connect(ui.buttonBox->button(QDialogButtonBox::Close), &QAbstractButton::clicked, this,
            &ServersManageDialog::onCloseButtonClick);
    connect(ui.tableWidget, &QTableWidget::cellDoubleClicked, this, &ServersManageDialog::onCellDoubleClick);

    auto tableHeader = ui.tableWidget->horizontalHeader();
    tableHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    tableHeader->setSectionResizeMode(1, QHeaderView::Stretch);
    tableHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

void ServersManageDialog::setServers(std::vector<std::shared_ptr<Server>> &servers) {
    this->servers = servers;

    ui.tableWidget->clearContents();
    for (auto &server : servers) {
        addServerRow(*server);
    }
}

std::vector<std::shared_ptr<Server>> &ServersManageDialog::getServers() { return servers; }

void ServersManageDialog::onAddButtonClick() {
    auto server = std::make_shared<Server>();
    auto dialog = std::make_unique<ServerEditDialog>(server, this);
    if (dialog->exec() == QDialog::DialogCode::Accepted) {
        servers.push_back(server);
        addServerRow(*server);
    }
}

void ServersManageDialog::onEditButtonClick() {
    auto select = ui.tableWidget->selectionModel();
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

void ServersManageDialog::onDeleteButtonClick() {
    auto select = ui.tableWidget->selectionModel();
    if (!select->hasSelection()) {
        return;
    }

    auto row = select->selectedRows().first().row();
    if (QMessageBox::warning(this, "確認", QString("接続先 %1 を削除してもよろしいですか？").arg(servers[row]->name),
                             QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok) {
        return;
    }

    ui.tableWidget->removeRow(row);
    servers.erase(servers.begin() + row);
}

void ServersManageDialog::onCloseButtonClick() { close(); }

void ServersManageDialog::onCellDoubleClick(int row, int column) {
    auto server = servers[row];
    auto dialog = std::make_unique<ServerEditDialog>(server, this);
    if (dialog->exec() == QDialog::DialogCode::Accepted) {
        setServerRow(row, *server);
    }
}

void ServersManageDialog::addServerRow(Server &server) {
    int row = ui.tableWidget->rowCount();
    ui.tableWidget->insertRow(row);
    setServerRow(row, server);
}

void ServersManageDialog::setServerRow(int row, Server &server) {
    ui.tableWidget->setItem(row, 0, new QTableWidgetItem(server.name));
    ui.tableWidget->setItem(row, 1, new QTableWidgetItem(server.address));

    auto useTLSItem = new QTableWidgetItem();
    useTLSItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    useTLSItem->setCheckState(server.useTLS ? Qt::Checked : Qt::Unchecked);
    ui.tableWidget->setItem(row, 2, useTLSItem);
}
