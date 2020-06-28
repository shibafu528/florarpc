#ifndef FLORARPC_DESCRIPTOR_POOL_PROXY_H
#define FLORARPC_DESCRIPTOR_POOL_PROXY_H

#include <google/protobuf/util/json_util.h>

#include <QJSEngine>
#include <QObject>

#include "entity/Method.h"

/**
 * JSからDescriptorPoolにアクセスするためのプロキシ
 */
class DescriptorPoolProxy : public QObject {
    Q_OBJECT

public:
    DescriptorPoolProxy(QJSEngine &js, const Method &method, QObject *parent = nullptr);
    Q_INVOKABLE QJSValue findMessageTypeByName(QString name);
    Q_INVOKABLE QJSValue findFieldByName(QString name);
    Q_INVOKABLE QJSValue findExtensionByName(QString name);
    Q_INVOKABLE QJSValue findOneOfByName(QString name);
    Q_INVOKABLE QJSValue findEnumTypeByName(QString name);
    Q_INVOKABLE QJSValue findEnumValueByName(QString name);
    Q_INVOKABLE QJSValue findServiceByName(QString name);
    Q_INVOKABLE QJSValue findMethodByName(QString name);

private:
    QJSValue parseFunction;
    const google::protobuf::DescriptorPool *pool;
};

#endif  // FLORARPC_DESCRIPTOR_POOL_PROXY_H
