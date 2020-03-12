#include "ImportsManageDialog.h"
#include <QFileDialog>
#include <QMessageBox>

ImportsManageDialog::ImportsManageDialog(QWidget *parent) : QDialog(parent) {
    ui.setupUi(this);

    connect(ui.browseButton, &QAbstractButton::clicked, this, &ImportsManageDialog::onBrowseButtonClick);
    connect(ui.addButton, &QAbstractButton::clicked, this, &ImportsManageDialog::onAddButtonClick);
    connect(ui.deleteButton, &QAbstractButton::clicked, this, &ImportsManageDialog::onDeleteButtonClick);
    connect(ui.buttonBox->button(QDialogButtonBox::Close), &QAbstractButton::clicked,
            this, &ImportsManageDialog::onCloseButtonClick);
}

void ImportsManageDialog::onBrowseButtonClick() {
    auto dirName = QFileDialog::getExistingDirectory(this, "インポート パスを追加");
    if (dirName.isEmpty()) {
        return;
    }

    ui.pathEdit->setText(dirName);
}

void ImportsManageDialog::onAddButtonClick() {
    if (ui.pathEdit->text().isEmpty()) {
        QMessageBox::warning(this, "エラー", "ディレクトリパスが入力されていません。");
        return;
    }

    QDir dir(ui.pathEdit->text());
    if (!dir.exists()) {
        QMessageBox::warning(this, "エラー", "無効なディレクトリパスが指定されています。");
        return;
    }

    if (!ui.list->findItems(dir.absolutePath(), Qt::MatchFlag::MatchExactly).isEmpty()) {
        QMessageBox::warning(this, "エラー", "すでに追加されています。");
        return;
    }

    ui.list->addItem(dir.absolutePath());
    ui.pathEdit->clear();
}

void ImportsManageDialog::onDeleteButtonClick() {
    if (ui.list->selectedItems().isEmpty()) {
        return;
    }

    delete ui.list->selectedItems().takeFirst();
}

void ImportsManageDialog::onCloseButtonClick() {
    close();
}
