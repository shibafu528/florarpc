#include "CertsEditDialog.h"

CertsEditDialog::CertsEditDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint) {
    ui.setupUi(this);

    connect(ui.buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this,
            &CertsEditDialog::onOkButtonClick);
    connect(ui.buttonBox->button(QDialogButtonBox::Cancel), &QAbstractButton::clicked, this,
            &CertsEditDialog::onCancelButtonClick);
}

void CertsEditDialog::onOkButtonClick() { done(Accepted); }

void CertsEditDialog::onCancelButtonClick() { done(Rejected); }
