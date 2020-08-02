#include "MetadataEdit.h"

#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>

#include "util/SyntaxHighlighter.h"

MetadataEdit::MetadataEdit(QWidget *parent)
    : QWidget(parent), validateTimer(new QTimer(this)), valid(true), metadata() {
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

    return metadata;
}

void MetadataEdit::onTextChanged() {
    validateTimer->start(std::chrono::seconds(3));
    emit changed();
}

void MetadataEdit::onValidateTimerTimeout() {
    valid = true;
    ui.error->setText("");
    ui.error->hide();
    metadata.clear();

    if (const auto metadataInput = ui.textEdit->toPlainText(); !metadataInput.isEmpty()) {
        QJsonParseError parseError = {};
        QJsonDocument metadataJson = QJsonDocument::fromJson(metadataInput.toUtf8(), &parseError);
        if (metadataJson.isNull()) {
            valid = false;
            ui.error->setText(QString("<b>Invalid metadata</b> %1").arg(parseError.errorString()));
            ui.error->show();
            return;
        }
        if (!metadataJson.isObject()) {
            valid = false;
            ui.error->setText("<b>Invalid metadata</b> Input must be a json object.");
            ui.error->show();
            return;
        }

        const QJsonObject &object = metadataJson.object();
        for (auto iter = object.constBegin(); iter != object.constEnd(); iter++) {
            const auto key = iter.key();
            const auto value = iter.value().toString();
            if (value == nullptr) {
                valid = false;
                metadata.clear();
                ui.error->setText(QString("<b>Invalid metadata</b> Metadata '%1' must be a string.").arg(key));
                ui.error->show();
                return;
            }

            if (key.endsWith("-bin")) {
                metadata.insert(key, QByteArray::fromBase64(value.toUtf8()));
            } else {
                metadata.insert(key, value);
            }
        }
    }
}
