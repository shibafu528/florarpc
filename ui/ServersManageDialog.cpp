#include "ServersManageDialog.h"

#include <QMessageBox>

#include "CertsEditDialog.h"
#include "ServerEditDialog.h"

ServersManageDialog::ServersManageDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint) {
    ui.setupUi(this);

    connect(ui.serversTable, &QTableWidget::cellDoubleClicked, this, &ServersManageDialog::onServerCellDoubleClick);
    connect(ui.addServerButton, &QAbstractButton::clicked, this, &ServersManageDialog::onAddServerButtonClick);
    connect(ui.editServerButton, &QAbstractButton::clicked, this, &ServersManageDialog::onEditServerButtonClick);
    connect(ui.deleteServerButton, &QAbstractButton::clicked, this, &ServersManageDialog::onDeleteServerButtonClick);
    connect(ui.addCertsButton, &QAbstractButton::clicked, this, &ServersManageDialog::onAddCertsButtonClick);
    connect(ui.editCertsButton, &QAbstractButton::clicked, this, &ServersManageDialog::onEditCertsButtonClick);
    connect(ui.deleteCertsButton, &QAbstractButton::clicked, this, &ServersManageDialog::onDeleteCertsButtonClick);
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

void ServersManageDialog::setCertificates(std::vector<std::shared_ptr<Certificate>> &certificates) {
    this->certificates = certificates;

    ui.certsTable->clearContents();
    for (auto &cert : certificates) {
        addCertsRow(*cert);
    }
}

std::vector<std::shared_ptr<Certificate>> &ServersManageDialog::getCertificates() { return certificates; }

void ServersManageDialog::onAddServerButtonClick() {
    auto server = std::make_shared<Server>();
    auto dialog = std::make_unique<ServerEditDialog>(server, certificates, this);
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
    auto dialog = std::make_unique<ServerEditDialog>(server, certificates, this);
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

void ServersManageDialog::onAddCertsButtonClick() {
    auto certs = std::make_shared<Certificate>();
    auto dialog = std::make_unique<CertsEditDialog>(certs, this);
    if (dialog->exec() == QDialog::DialogCode::Accepted) {
        certificates.push_back(certs);
        addCertsRow(*certs);
    }
}

void ServersManageDialog::onEditCertsButtonClick() {
    auto select = ui.certsTable->selectionModel();
    if (!select->hasSelection()) {
        return;
    }

    auto row = select->selectedRows().first();
    auto certificate = certificates[row.row()];
    auto dialog = std::make_unique<CertsEditDialog>(certificate, this);
    if (dialog->exec() == QDialog::DialogCode::Accepted) {
        setCertsRow(row.row(), *certificate);
    }
}

void ServersManageDialog::onDeleteCertsButtonClick() {
    auto select = ui.certsTable->selectionModel();
    if (!select->hasSelection()) {
        return;
    }

    auto row = select->selectedRows().first().row();
    if (QMessageBox::warning(this, "確認",
                             QString("証明書 %1 を削除してもよろしいですか？").arg(certificates[row]->name),
                             QMessageBox::Ok | QMessageBox::Cancel) != QMessageBox::Ok) {
        return;
    }

    ui.certsTable->removeRow(row);
    certificates.erase(certificates.begin() + row);
}

void ServersManageDialog::onCloseButtonClick() { close(); }

void ServersManageDialog::onServerCellDoubleClick(int row, int column) {
    auto server = servers[row];
    auto dialog = std::make_unique<ServerEditDialog>(server, certificates, this);
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

void ServersManageDialog::addCertsRow(Certificate &certificate) {
    int row = ui.certsTable->rowCount();
    ui.certsTable->insertRow(row);
    setCertsRow(row, certificate);
}

void ServersManageDialog::setCertsRow(int row, Certificate &certificate) {
    ui.certsTable->setItem(row, 0, new QTableWidgetItem(certificate.name));
    ui.certsTable->setItem(row, 1, new QTableWidgetItem(certificate.rootCertsName));
    ui.certsTable->setItem(row, 2, new QTableWidgetItem(certificate.certChainName));
    ui.certsTable->setItem(row, 3, new QTableWidgetItem(certificate.privateKeyName));
}
