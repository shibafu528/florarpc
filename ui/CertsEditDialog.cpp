#include "CertsEditDialog.h"

#include <QMessageBox>

CertsEditDialog::CertsEditDialog(std::shared_ptr<Certificate> certificate, QWidget *parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
      certificate(std::move(certificate)) {
    ui.setupUi(this);

    connect(ui.buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this,
            &CertsEditDialog::onOkButtonClick);
    connect(ui.buttonBox->button(QDialogButtonBox::Cancel), &QAbstractButton::clicked, this,
            &CertsEditDialog::onCancelButtonClick);
    connect(ui.targetNameOverrideCheck, &QCheckBox::stateChanged, this,
            &CertsEditDialog::onTargetNameOverrideCheckChanged);

    ui.rootCertsControl->setAcceptType(CertsEditControl::AcceptType::Certificate);
    ui.privateKeyControl->setAcceptType(CertsEditControl::AcceptType::RSAPrivateKey);
    ui.certChainControl->setAcceptType(CertsEditControl::AcceptType::Certificate);

    ui.nameEdit->setText(this->certificate->name);
    ui.rootCertsControl->setFilePath(this->certificate->rootCertsPath);
    ui.rootCertsControl->setFilename(this->certificate->rootCertsName);
    ui.privateKeyControl->setFilePath(this->certificate->privateKeyPath);
    ui.privateKeyControl->setFilename(this->certificate->privateKeyName);
    ui.certChainControl->setFilePath(this->certificate->certChainPath);
    ui.certChainControl->setFilename(this->certificate->certChainName);
    if (!this->certificate->targetNameOverride.isEmpty()) {
        ui.targetNameOverrideCheck->setChecked(true);
        ui.targetNameOverrideEdit->setText(this->certificate->targetNameOverride);
    }
}

void CertsEditDialog::onOkButtonClick() {
    if (ui.nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "接続先名が入力されていません。");
        return;
    }

    if (ui.rootCertsControl->getFilename().isEmpty()) {
        QMessageBox::warning(this, "Error", "ルート証明書の指定は必須です。");
        return;
    }

    certificate->name = ui.nameEdit->text();
    certificate->rootCertsPath = ui.rootCertsControl->getFilePath();
    certificate->rootCertsName = ui.rootCertsControl->getFilename();
    certificate->privateKeyPath = ui.privateKeyControl->getFilePath();
    certificate->privateKeyName = ui.privateKeyControl->getFilename();
    certificate->certChainPath = ui.certChainControl->getFilePath();
    certificate->certChainName = ui.certChainControl->getFilename();
    if (ui.targetNameOverrideCheck->isChecked()) {
        certificate->targetNameOverride = ui.targetNameOverrideEdit->text();
    } else {
        certificate->targetNameOverride = "";
    }

    done(Accepted);
}

void CertsEditDialog::onCancelButtonClick() { done(Rejected); }

void CertsEditDialog::onTargetNameOverrideCheckChanged(int state) {
    ui.targetNameOverrideEdit->setEnabled(state == Qt::CheckState::Checked);
}
