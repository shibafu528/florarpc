#ifndef FLORARPC_METADATA_H
#define FLORARPC_METADATA_H

#include <QHash>
#include <QMultiMap>

class Metadata {
    typedef QMultiMap<QString, QString> Container;

public:
    enum class MergeStrategy {
        Preserve,
        Replace,
    };

    QString parseJson(const QString &input, MergeStrategy mergeStrategy = MergeStrategy::Preserve);
    inline QMultiMap<QString, QString> getValues() const { return data; }
    QHash<QString, QString> asHash();

private:
    Container data;
};

#endif  // FLORARPC_METADATA_H
