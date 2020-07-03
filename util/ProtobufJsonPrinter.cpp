#include "ProtobufJsonPrinter.h"

#include <google/protobuf/descriptor.h>

#include <minijson_writer.hpp>
#include <sstream>

#include "ProtobufIterator.h"

template <typename T>
static void write2json(minijson::object_writer &writer, const std::string &name, bool repeated, const T &value);

static void field2json(minijson::object_writer &writer, const google::protobuf::Descriptor *descriptor,
                       const std::string &key, const google::protobuf::FieldDescriptor *valueField,
                       std::vector<const google::protobuf::Descriptor *> &path);

static void desc2json(minijson::object_writer &writer, const google::protobuf::Descriptor *descriptor,
                      std::vector<const google::protobuf::Descriptor *> &path);

template <typename T>
static void write2json(minijson::object_writer &writer, const std::string &name, bool repeated, const T &value) {
    if (repeated) {
        auto array = writer.nested_array(name.c_str());
        array.write(value);
        array.close();
    } else {
        writer.write(name.c_str(), value);
    }
}

static void field2json(minijson::object_writer &writer, const google::protobuf::Descriptor *descriptor,
                       const std::string &key, const google::protobuf::FieldDescriptor *valueField,
                       std::vector<const google::protobuf::Descriptor *> &path) {
    switch (valueField->type()) {
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
            write2json(writer, key, valueField->is_repeated(), valueField->default_value_double());
            break;
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
            write2json(writer, key, valueField->is_repeated(), valueField->default_value_float());
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT64:
        case google::protobuf::FieldDescriptor::TYPE_FIXED64:
        case google::protobuf::FieldDescriptor::TYPE_SINT64:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
            write2json(writer, key, valueField->is_repeated(), valueField->default_value_int64());
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
            write2json(writer, key, valueField->is_repeated(), valueField->default_value_uint64());
            break;
        case google::protobuf::FieldDescriptor::TYPE_INT32:
        case google::protobuf::FieldDescriptor::TYPE_FIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
        case google::protobuf::FieldDescriptor::TYPE_SINT32:
            write2json(writer, key, valueField->is_repeated(), valueField->default_value_int32());
            break;
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
            write2json(writer, key, valueField->is_repeated(), valueField->default_value_uint32());
            break;
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
            write2json(writer, key, valueField->is_repeated(), valueField->default_value_bool());
            break;
        case google::protobuf::FieldDescriptor::TYPE_STRING:
            write2json(writer, key, valueField->is_repeated(), valueField->default_value_string());
            break;
        case google::protobuf::FieldDescriptor::TYPE_GROUP:
            break;
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
            // TODO: better well-known type supports (e.g. google.protobuf.Timestamp)
            path.push_back(descriptor);
            if (valueField->is_repeated()) {
                auto array = writer.nested_array(key.c_str());
                auto message = array.nested_object();
                desc2json(message, valueField->message_type(), path);
                message.close();
                array.close();
            } else {
                auto message = writer.nested_object(key.c_str());
                desc2json(message, valueField->message_type(), path);
                message.close();
            }
            path.pop_back();
            break;
        }
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
            writer.write(key.c_str(), "");
            break;
        case google::protobuf::FieldDescriptor::TYPE_ENUM:
            writer.write(key.c_str(), valueField->default_value_enum()->name());
            break;
    }
}

static void desc2json(minijson::object_writer &writer, const google::protobuf::Descriptor *descriptor,
                      std::vector<const google::protobuf::Descriptor *> &path) {
    // stop if detected recursive message
    for (auto desc : path) {
        if (desc == descriptor) {
            return;
        }
    }

    ProtobufIterator::Iterable<google::protobuf::Descriptor, google::protobuf::FieldDescriptor> rootFields(descriptor);
    for (auto field : rootFields) {
        if (field->is_map()) {
            const auto key =
                field->message_type()->FindFieldByNumber(1)->type() == google::protobuf::FieldDescriptor::TYPE_STRING
                    ? "key"
                    : "0";
            const auto valueField = field->message_type()->FindFieldByNumber(2);

            auto object = writer.nested_object(field->camelcase_name().c_str());
            field2json(object, descriptor, key, valueField, path);
            object.close();
        } else {
            field2json(writer, descriptor, field->camelcase_name(), field, path);
        }
    }
}

std::string ProtobufJsonPrinter::makeRequestSkeleton(const google::protobuf::Descriptor *descriptor) {
    std::ostringstream stream;
    minijson::object_writer writer(stream,
                                   minijson::writer_configuration().pretty_printing(true).indent_spaces(2));
    std::vector<const google::protobuf::Descriptor *> path;
    desc2json(writer, descriptor, path);
    writer.close();
    return std::string(stream.str());
}
