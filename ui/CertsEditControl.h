#ifndef FLORARPC_CERTSEDITCONTROL_H
#define FLORARPC_CERTSEDITCONTROL_H

#include <QWidget>

#include "ui/ui_CertsEditControl.h"

class CertsEditControl : public QWidget {
    Q_OBJECT

public:
    explicit CertsEditControl(QWidget *parent = nullptr);

private:
    Ui_CertsEditControl ui;
};

#endif  // FLORARPC_CERTSEDITCONTROL_H
