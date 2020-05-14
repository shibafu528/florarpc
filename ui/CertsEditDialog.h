#ifndef FLORARPC_CERTSEDITDIALOG_H
#define FLORARPC_CERTSEDITDIALOG_H

#include <QDialog>

#include "entity/Certificate.h"
#include "ui/ui_CertsEditDialog.h"

class CertsEditDialog : public QDialog {
    Q_OBJECT

public:
    CertsEditDialog(std::shared_ptr<Certificate> certificate, QWidget *parent = nullptr);

private slots:
    void onOkButtonClick();

    void onCancelButtonClick();

private:
    Ui_CertsEditDialog ui;
    std::shared_ptr<Certificate> certificate;
};

#endif  // FLORARPC_CERTSEDITDIALOG_H
