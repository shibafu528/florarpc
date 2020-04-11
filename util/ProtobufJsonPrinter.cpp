#include <sstream>
#include <google/protobuf/descriptor.h>
#include <minijson_writer.hpp>
#include <sstream>
#include "ProtobufIterator.h"
#include "ProtobufJsonPrinter.h"

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

static void desc2json(minijson::object_writer &writer, const google::protobuf::Descriptor *descriptor, std::vector<const google::protobuf::Descriptor*> &path) {
    // stop if detected recursive message
    for (auto desc : path) {
        if (desc == descriptor) {
            return;
        }
    }

    ProtobufIterator::Iterable<google::protobuf::FieldDescriptor> rootFields(descriptor);
    for (auto field : rootFields) {
        // TODO: better map support
        switch (field->type()) {
            case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
                write2json(writer, field->camelcase_name(), field->is_repeated(), field->default_value_double());
                break;
            case google::protobuf::FieldDescriptor::TYPE_FLOAT:
                write2json(writer, field->camelcase_name(), field->is_repeated(), field->default_value_float());
                break;
            case google::protobuf::FieldDescriptor::TYPE_INT64:
            case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            case google::protobuf::FieldDescriptor::TYPE_SINT64:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
                write2json(writer, field->camelcase_name(), field->is_repeated(), field->default_value_int64());
                break;
            case google::protobuf::FieldDescriptor::TYPE_UINT64:
                write2json(writer, field->camelcase_name(), field->is_repeated(), field->default_value_uint64());
                break;
            case google::protobuf::FieldDescriptor::TYPE_INT32:
            case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
            case google::protobuf::FieldDescriptor::TYPE_SINT32:
                write2json(writer, field->camelcase_name(), field->is_repeated(), field->default_value_int32());
                break;
            case google::protobuf::FieldDescriptor::TYPE_UINT32:
                write2json(writer, field->camelcase_name(), field->is_repeated(), field->default_value_uint32());
                break;
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                write2json(writer, field->camelcase_name(), field->is_repeated(), field->default_value_bool());
                break;
            case google::protobuf::FieldDescriptor::TYPE_STRING:
                write2json(writer, field->camelcase_name(), field->is_repeated(), field->default_value_string());
                break;
            case google::protobuf::FieldDescriptor::TYPE_GROUP:
                break;
            case google::protobuf::FieldDescriptor::TYPE_MESSAGE: {
                // TODO: better well-known type supports (e.g. google.protobuf.Timestamp)
                path.push_back(descriptor);
                if (field->is_repeated()) {
                    auto array = writer.nested_array(field->camelcase_name().c_str());
                    auto message = array.nested_object();
                    desc2json(message, field->message_type(), path);
                    message.close();
                    array.close();
                } else {
                    auto message = writer.nested_object(field->camelcase_name().c_str());
                    desc2json(message, field->message_type(), path);
                    message.close();
                }
                path.pop_back();
                break;
            }
            case google::protobuf::FieldDescriptor::TYPE_BYTES:
                writer.write(field->camelcase_name().c_str(), "");
                break;
            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                writer.write(field->camelcase_name().c_str(), field->default_value_enum()->name());
                break;
        }
    }
}

std::string ProtobufJsonPrinter::makeRequestSkeleton(const google::protobuf::Descriptor *descriptor) {
    std::ostringstream stream;
    minijson::object_writer writer(stream,
                                   minijson::writer_configuration().pretty_printing(true).indent_spaces(2));
    std::vector<const google::protobuf::Descriptor*> path;
    desc2json(writer, descriptor, path);
    writer.close();
    return std::string(stream.str());
}
