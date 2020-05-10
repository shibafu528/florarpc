#ifndef FLORARPC_CERTSEDITDIALOG_H
#define FLORARPC_CERTSEDITDIALOG_H

#include <QDialog>

#include "ui/ui_CertsEditDialog.h"

class CertsEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit CertsEditDialog(QWidget *parent = nullptr);

private slots:
    void onOkButtonClick();

    void onCancelButtonClick();

private:
    Ui_CertsEditDialog ui;
};

#endif  // FLORARPC_CERTSEDITDIALOG_H
