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
    connect(ui.targetNameOverrideCheck, &QCheckBox::stateChanged, this,
            &ServerEditDialog::onTargetNameOverrideCheckChanged);
    connect(ui.buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this,
            &ServerEditDialog::onOkButtonClick);
    connect(ui.buttonBox->button(QDialogButtonBox::Cancel), &QAbstractButton::clicked, this,
            &ServerEditDialog::onCancelButtonClick);

    ui.nameEdit->setText(this->server->name);
    ui.addressEdit->setText(this->server->address);
    ui.useTLSCheck->setChecked(this->server->useTLS);
    ui.sharedMetadataEdit->setString(this->server->sharedMetadata);

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

    if (!this->server->tlsTargetNameOverride.isEmpty()) {
        ui.targetNameOverrideCheck->setChecked(true);
        ui.targetNameOverrideEdit->setText(this->server->tlsTargetNameOverride);
    }
}

void ServerEditDialog::onUseTLSCheckChanged(int state) {
    auto checked = state == Qt::CheckState::Checked;
    ui.certificateComboBox->setEnabled(checked);
    ui.targetNameOverrideCheck->setEnabled(checked);
    ui.targetNameOverrideHelp->setEnabled(checked);
    ui.targetNameOverrideEdit->setEnabled(checked && ui.targetNameOverrideCheck->isChecked());
}

void ServerEditDialog::onTargetNameOverrideCheckChanged(int state) {
    ui.targetNameOverrideEdit->setEnabled(state == Qt::CheckState::Checked && ui.useTLSCheck->isChecked());
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

    if (!ui.sharedMetadataEdit->isValid()) {
        QMessageBox::warning(this, "Error", "共通メタデータに入力エラーがあります。");
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
    server->sharedMetadata = ui.sharedMetadataEdit->toString();

    if (ui.targetNameOverrideCheck->isChecked()) {
        server->tlsTargetNameOverride = ui.targetNameOverrideEdit->text();
    } else {
        server->tlsTargetNameOverride = "";
    }

    done(Accepted);
}

void ServerEditDialog::onCancelButtonClick() { done(Rejected); }
