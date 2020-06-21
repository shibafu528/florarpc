#ifndef FLORARPC_ABOUTDIALOG_H
#define FLORARPC_ABOUTDIALOG_H

#include <QDialog>

#include "ui/ui_AboutDialog.h"

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);

private slots:
    void onOkButtonClick();

private:
    Ui_AboutDialog ui;
};

#endif  // FLORARPC_ABOUTDIALOG_H
