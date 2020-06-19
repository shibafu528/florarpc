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

    void setFilePath(QString filePath);

    QString getFilePath();

    void setFilename(QString filename);

    QString getFilename();

    void setAcceptType(AcceptType acceptType);

private slots:
    void onImportButtonClick();

    void onDeleteButtonClick();

private:
    Ui_CertsEditControl ui;
    QString filePath;
    QString filename;
    AcceptType acceptType;

    void updateState();
};

#endif  // FLORARPC_CERTSEDITCONTROL_H
