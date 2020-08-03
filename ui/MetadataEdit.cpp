#include "MetadataEdit.h"

#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>

#include "util/SyntaxHighlighter.h"

MetadataEdit::MetadataEdit(QWidget *parent)
    : QWidget(parent), validateTimer(new QTimer(this)), valid(true), metadata(nullptr) {
    ui.setupUi(this);

    connect(ui.textEdit, &QTextEdit::textChanged, this, &MetadataEdit::onTextChanged);
    connect(validateTimer, &QTimer::timeout, this, &MetadataEdit::onValidateTimerTimeout);

    highlighter = SyntaxHighlighter::setup(*ui.textEdit, palette());

    const auto fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui.textEdit->setFont(fixedFont);
    ui.error->hide();

    validateTimer->setSingleShot(true);
}

bool MetadataEdit::isValid() {
    if (validateTimer->isActive()) {
        validateTimer->stop();
        onValidateTimerTimeout();
    }

    return valid;
}

QString MetadataEdit::toString() { return ui.textEdit->toPlainText(); }

void MetadataEdit::setString(const QString &metadata) { ui.textEdit->setText(metadata); }

Session::Metadata MetadataEdit::toMap() {
    if (validateTimer->isActive()) {
        validateTimer->stop();
        onValidateTimerTimeout();
    }

    if (metadata) {
        return metadata->getValues();
    } else {
        return Session::Metadata();
    }
}

void MetadataEdit::onTextChanged() {
    validateTimer->start(std::chrono::seconds(3));
    emit changed();
}

void MetadataEdit::onValidateTimerTimeout() {
    valid = true;
    ui.error->setText("");
    ui.error->hide();
    metadata = std::make_unique<Metadata>();

    const auto metadataInput = ui.textEdit->toPlainText();
    if (const auto error = metadata->parseJson(metadataInput); !error.isEmpty()) {
        valid = false;
        metadata.reset();
        ui.error->setText(QString("<b>Invalid metadata</b> %1").arg(error));
        ui.error->show();
    }
}
