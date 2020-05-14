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
    ui.privateKeyControl->setPem(this->certificate->privateKey);
    ui.certChainControl->setPem(this->certificate->certChain);
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
    certificate->privateKey = ui.privateKeyControl->getPem();
    certificate->certChain = ui.certChainControl->getPem();

    done(Accepted);
}

void CertsEditDialog::onCancelButtonClick() { done(Rejected); }
