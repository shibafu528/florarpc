#include "CertsEditControl.h"

#include <openssl/pem.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

CertsEditControl::CertsEditControl(QWidget *parent) : QWidget(parent), acceptType(AcceptType::Unknown) {
    ui.setupUi(this);

    connect(ui.importButton, &QAbstractButton::clicked, this, &CertsEditControl::onImportButtonClick);
    connect(ui.deleteButton, &QAbstractButton::clicked, this, &CertsEditControl::onDeleteButtonClick);
}

void CertsEditControl::setFilePath(QString filePath) {
    this->filePath = filePath;
    updateState();
}

QString CertsEditControl::getFilePath() { return filePath; }

void CertsEditControl::setFilename(QString filename) {
    this->filename = filename;
    this->ui.filenameLabel->setText(filename);
    updateState();
}

QString CertsEditControl::getFilename() { return filename; }

void CertsEditControl::setAcceptType(CertsEditControl::AcceptType acceptType) { this->acceptType = acceptType; }

void CertsEditControl::onImportButtonClick() {
    Q_ASSERT(acceptType != AcceptType::Unknown);

    const auto baseDirectory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const auto filename = QFileDialog::getOpenFileName(this, "Open PEM file", baseDirectory);
    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(this, "Import error",
                              "ファイルの読み込み中にエラーが発生しました。\nファイルを開けません。");
        return;
    }

    const auto bin = file.readAll();
    file.close();

    BIO *bio = BIO_new_mem_buf(bin.constData(), bin.length());
    char *name, *header;
    unsigned char *data;
    long dataLength;
    int readResult = PEM_read_bio(bio, &name, &header, &data, &dataLength);
    BIO_free(bio);
    if (readResult == 0) {
        QMessageBox::critical(this, "Import error",
                              "ファイルの読み込み中にエラーが発生しました。\n有効なPEM形式のファイルではありません。");
        return;
    }

    const auto qName = QLatin1String(name);
    const auto acceptable = (acceptType == AcceptType::Certificate && qName == PEM_STRING_X509) ||
                            (acceptType == AcceptType::RSAPrivateKey && qName == PEM_STRING_RSA);
    if (acceptable) {
        QFileInfo fileInfo(filename);
        setFilePath(fileInfo.absoluteFilePath());
        setFilename(fileInfo.fileName());
    } else {
        switch (acceptType) {
            case AcceptType::Unknown:
                Q_ASSERT(false);
                break;
            case AcceptType::Certificate:
                QMessageBox::critical(
                    this, "Import error",
                    "ファイルの読み込み中にエラーが発生しました。\n証明書が格納されたPEMファイルを指定してください。");
                break;
            case AcceptType::RSAPrivateKey:
                QMessageBox::critical(
                    this, "Import error",
                    "ファイルの読み込み中にエラーが発生しました。\n秘密鍵が格納されたPEMファイルを指定してください。");
                break;
        }
    }

    OPENSSL_free(name);
    OPENSSL_free(header);
    OPENSSL_free(data);
}

void CertsEditControl::onDeleteButtonClick() {
    setFilePath("");
    setFilename("");
}

void CertsEditControl::updateState() {
    if (filePath.isEmpty() && filename.isEmpty()) {
        ui.importButton->show();
        ui.deleteButton->hide();
    } else {
        ui.importButton->hide();
        ui.deleteButton->show();
    }
}
