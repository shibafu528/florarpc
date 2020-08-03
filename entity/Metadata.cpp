#include "Metadata.h"

#include <QJsonDocument>
#include <QJsonObject>

QString Metadata::parseJson(const QString& input) {
    if (!input.isEmpty()) {
        QJsonParseError parseError = {};
        QJsonDocument metadataJson = QJsonDocument::fromJson(input.toUtf8(), &parseError);
        if (metadataJson.isNull()) {
            return parseError.errorString();
        }
        if (!metadataJson.isObject()) {
            return "Metadata input must be an object.";
        }

        const QJsonObject& object = metadataJson.object();
        for (auto iter = object.constBegin(); iter != object.constEnd(); iter++) {
            const auto key = iter.key();
            const auto value = iter.value().toString();
            if (value == nullptr) {
                return QString("Metadata '%1' must be a string.").arg(key);
            }

            if (key.endsWith("-bin")) {
                data.insert(key, QByteArray::fromBase64(value.toUtf8()));
            } else {
                data.insert(key, value);
            }
        }
    }
    return QString();
}

QHash<QString, QString> Metadata::asHash() {
    QHash<QString, QString> hash;

    for (auto iter = data.cbegin(); iter != data.cend(); iter++) {
        hash.insert(iter.key(), iter.value());
    }

    return hash;
}
