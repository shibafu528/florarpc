#ifndef FLORARPC_METADATA_H
#define FLORARPC_METADATA_H

#include <QHash>
#include <QMultiMap>

class Metadata {
public:
    QString parseJson(const QString &input);
    inline QMultiMap<QString, QString> getValues() const { return data; }
    QHash<QString, QString> asHash();

private:
    QMultiMap<QString, QString> data;
};

#endif  // FLORARPC_METADATA_H
