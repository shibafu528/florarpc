#ifndef FLORARPC_IMPORTSMANAGEDIALOG_H
#define FLORARPC_IMPORTSMANAGEDIALOG_H

#include <QDialog>
#include "ui/ui_ImportsManageDialog.h"

class ImportsManageDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportsManageDialog(QWidget *parent = nullptr);

private:
    Ui::ImportsManageDialog ui;
};


#endif //FLORARPC_IMPORTSMANAGEDIALOG_H
