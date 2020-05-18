#include "ServerEditDialog.h"

#include <QMessageBox>
#include <QPushButton>

ServerEditDialog::ServerEditDialog(std::shared_ptr<Server> server,
                                   std::vector<std::shared_ptr<Certificate>> certificates, QWidget *parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
      server(std::move(server)),
      certificates(std::move(certificates)) {
    ui.setupUi(this);

    connect(ui.useTLSCheck, &QCheckBox::stateChanged, this, &ServerEditDialog::onUseTLSCheckChanged);
    connect(ui.buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this,
            &ServerEditDialog::onOkButtonClick);
    connect(ui.buttonBox->button(QDialogButtonBox::Cancel), &QAbstractButton::clicked, this,
            &ServerEditDialog::onCancelButtonClick);

    ui.nameEdit->setText(this->server->name);
    ui.addressEdit->setText(this->server->address);
    ui.useTLSCheck->setChecked(this->server->useTLS);

    ui.certificateComboBox->setEnabled(this->server->useTLS);
    ui.certificateComboBox->addItem("(指定なし)");
    for (auto &cert : this->certificates) {
        ui.certificateComboBox->addItem(cert->name, cert->id);
    }
    if (!this->server->certificateUUID.isNull()) {
        int certificateIndex = ui.certificateComboBox->findData(this->server->certificateUUID);
        if (certificateIndex != -1) {
            ui.certificateComboBox->setCurrentIndex(certificateIndex);
        }
    }
}

void ServerEditDialog::onUseTLSCheckChanged(int state) {
    ui.certificateComboBox->setEnabled(state == Qt::CheckState::Checked);
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
    if (!ui.certificateComboBox->currentData().isNull()) {
        server->certificateUUID = ui.certificateComboBox->currentData().toUuid();
    } else {
        server->certificateUUID = QUuid();
    }

    done(Accepted);
}

void ServerEditDialog::onCancelButtonClick() { done(Rejected); }
