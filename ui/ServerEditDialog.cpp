#include "ServerEditDialog.h"

#include <QMessageBox>
#include <QPushButton>

ServerEditDialog::ServerEditDialog(std::shared_ptr<Server> server, QWidget *parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
      server(std::move(server)) {
    ui.setupUi(this);

    connect(ui.buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this,
            &ServerEditDialog::onOkButtonClick);
    connect(ui.buttonBox->button(QDialogButtonBox::Cancel), &QAbstractButton::clicked, this,
            &ServerEditDialog::onCancelButtonClick);

    ui.nameEdit->setText(this->server->name);
    ui.addressEdit->setText(this->server->address);
    ui.useTLSCheck->setChecked(this->server->useTLS);
}

void ServerEditDialog::onOkButtonClick() {
    if (ui.nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "接続先名が入力されていません。");
        return;
    }

    if (ui.addressEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "アドレスが入力されていません。");
        return;
    }

    server->name = ui.nameEdit->text();
    server->address = ui.addressEdit->text();
    server->useTLS = ui.useTLSCheck->isChecked();

    done(Accepted);
}

void ServerEditDialog::onCancelButtonClick() { done(Rejected); }
