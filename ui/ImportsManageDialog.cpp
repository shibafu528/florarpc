#include "ImportsManageDialog.h"

ImportsManageDialog::ImportsManageDialog(QWidget *parent) : QDialog(parent) {
    ui.setupUi(this);

    connect(ui.buttonBox->button(QDialogButtonBox::Close), &QAbstractButton::clicked, [this]() {
        close();
    });
}
