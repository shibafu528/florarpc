#include "DescriptorPoolProxy.h"

template <typename TProto, typename TDesc>
static QJSValueList protoToArgs(TDesc desc) {
    TProto proto;
    desc->CopyTo(&proto);
    std::string json;
    google::protobuf::util::MessageToJsonString(proto, &json);
    return QJSValueList({QString::fromStdString(json)});
}

DescriptorPoolProxy::DescriptorPoolProxy(QJSEngine &js, const Method &method, QObject *parent)
    : QObject(parent), parseFunction(js.evaluate("JSON.parse")), pool(method.descriptor->file()->pool()) {}

QJSValue DescriptorPoolProxy::findMessageTypeByName(QString name) {
    const auto desc = pool->FindMessageTypeByName(name.toStdString());
    if (desc == nullptr) {
        return QJSValue(QJSValue::NullValue);
    }
    return parseFunction.call(protoToArgs<google::protobuf::DescriptorProto>(desc));
}

QJSValue DescriptorPoolProxy::findFieldByName(QString name) {
    const auto desc = pool->FindFieldByName(name.toStdString());
    if (desc == nullptr) {
        return QJSValue(QJSValue::NullValue);
    }
    return parseFunction.call(protoToArgs<google::protobuf::FieldDescriptorProto>(desc));
}

QJSValue DescriptorPoolProxy::findExtensionByName(QString name) {
    const auto desc = pool->FindExtensionByName(name.toStdString());
    if (desc == nullptr) {
        return QJSValue(QJSValue::NullValue);
    }
    return parseFunction.call(protoToArgs<google::protobuf::FieldDescriptorProto>(desc));
}

QJSValue DescriptorPoolProxy::findOneOfByName(QString name) {
    const auto desc = pool->FindOneofByName(name.toStdString());
    if (desc == nullptr) {
        return QJSValue(QJSValue::NullValue);
    }
    return parseFunction.call(protoToArgs<google::protobuf::OneofDescriptorProto>(desc));
}

QJSValue DescriptorPoolProxy::findEnumTypeByName(QString name) {
    const auto desc = pool->FindEnumTypeByName(name.toStdString());
    if (desc == nullptr) {
        return QJSValue(QJSValue::NullValue);
    }
    return parseFunction.call(protoToArgs<google::protobuf::EnumDescriptorProto>(desc));
}

QJSValue DescriptorPoolProxy::findEnumValueByName(QString name) {
    const auto desc = pool->FindEnumValueByName(name.toStdString());
    if (desc == nullptr) {
        return QJSValue(QJSValue::NullValue);
    }
    return parseFunction.call(protoToArgs<google::protobuf::EnumValueDescriptorProto>(desc));
}

QJSValue DescriptorPoolProxy::findServiceByName(QString name) {
    const auto desc = pool->FindServiceByName(name.toStdString());
    if (desc == nullptr) {
        return QJSValue(QJSValue::NullValue);
    }
    return parseFunction.call(protoToArgs<google::protobuf::ServiceDescriptorProto>(desc));
}

QJSValue DescriptorPoolProxy::findMethodByName(QString name) {
    const auto desc = pool->FindMethodByName(name.toStdString());
    if (desc == nullptr) {
        return QJSValue(QJSValue::NullValue);
    }
    return parseFunction.call(protoToArgs<google::protobuf::MethodDescriptorProto>(desc));
}
