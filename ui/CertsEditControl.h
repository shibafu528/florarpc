#ifndef FLORARPC_CERTSEDITCONTROL_H
#define FLORARPC_CERTSEDITCONTROL_H

#include <QByteArray>
#include <QWidget>

#include "ui/ui_CertsEditControl.h"

class CertsEditControl : public QWidget {
    Q_OBJECT

public:
    enum class AcceptType {
        Unknown,
        Certificate,
        RSAPrivateKey,
    };

    explicit CertsEditControl(QWidget *parent = nullptr);

    void setPem(QByteArray pem);

    QByteArray getPem();

    void setAcceptType(AcceptType acceptType);

private slots:
    void onImportButtonClick();

    void onShowButtonClick();

    void onDeleteButtonClick();

private:
    Ui_CertsEditControl ui;
    QByteArray pem;
    AcceptType acceptType;
};

#endif  // FLORARPC_CERTSEDITCONTROL_H
