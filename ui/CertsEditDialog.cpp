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

    ui.rootCertsControl->setAcceptType(CertsEditControl::AcceptType::Certificate);
    ui.privateKeyControl->setAcceptType(CertsEditControl::AcceptType::RSAPrivateKey);
    ui.certChainControl->setAcceptType(CertsEditControl::AcceptType::Certificate);

    ui.nameEdit->setText(this->certificate->name);
    ui.rootCertsControl->setPem(this->certificate->rootCerts);
    ui.rootCertsControl->setFilename(this->certificate->rootCertsName);
    ui.privateKeyControl->setPem(this->certificate->privateKey);
    ui.privateKeyControl->setFilename(this->certificate->privateKeyName);
    ui.certChainControl->setPem(this->certificate->certChain);
    ui.certChainControl->setFilename(this->certificate->certChainName);
}

void CertsEditDialog::onOkButtonClick() {
    if (ui.nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "接続先名が入力されていません。");
        return;
    }

    if (ui.rootCertsControl->getPem().isEmpty()) {
        QMessageBox::warning(this, "Error", "ルート証明書のインポートは必須です。");
        return;
    }

    certificate->name = ui.nameEdit->text();
    certificate->rootCerts = ui.rootCertsControl->getPem();
    certificate->rootCertsName = ui.rootCertsControl->getFilename();
    certificate->privateKey = ui.privateKeyControl->getPem();
    certificate->privateKeyName = ui.privateKeyControl->getFilename();
    certificate->certChain = ui.certChainControl->getPem();
    certificate->certChainName = ui.certChainControl->getFilename();

    done(Accepted);
}

void CertsEditDialog::onCancelButtonClick() { done(Rejected); }
