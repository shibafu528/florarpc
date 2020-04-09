#include "Method.h"
#include "../util/GrpcUtility.h"
#include "../util/ProtobufJsonPrinter.h"
#include <sstream>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>

Method::Method(const google::protobuf::MethodDescriptor *descriptor) : descriptor(descriptor) {}

const std::string &Method::getFullName() const {
    return descriptor->full_name();
}

std::string Method::getRequestPath() const {
    std::stringstream str;
    str << "/" << descriptor->service()->full_name() << "/" << descriptor->name();
    return std::string(str.str());
}

std::string Method::makeRequestSkeleton() {
    return ProtobufJsonPrinter::makeRequestSkeleton(descriptor->input_type());
}

std::unique_ptr<google::protobuf::Message>
Method::parseRequest(google::protobuf::DynamicMessageFactory &factory, const std::string &json) {
    auto reqProto = factory.GetPrototype(descriptor->input_type());
    auto reqMessage = std::unique_ptr<google::protobuf::Message>(reqProto->New());
    google::protobuf::util::JsonParseOptions parseOptions;
    parseOptions.ignore_unknown_fields = true;
    parseOptions.case_insensitive_enum_parsing = true;
    auto parseStatus = google::protobuf::util::JsonStringToMessage(json, reqMessage.get(), parseOptions);
    if (!parseStatus.ok()) {
        throw ParseError(std::make_unique<std::string>(parseStatus.error_message()));
    }
    return reqMessage;
}

std::unique_ptr<google::protobuf::Message>
Method::parseResponse(google::protobuf::DynamicMessageFactory &factory, const grpc::ByteBuffer &buffer) {
    auto resProto = factory.GetPrototype(descriptor->output_type());
    auto resMessage = std::unique_ptr<google::protobuf::Message>(resProto->New());
    GrpcUtility::parseMessage(buffer, *resMessage);
    return resMessage;
}

Method::ParseError::ParseError(std::unique_ptr<std::string> message) : message(std::move(message)) {}

const std::string &Method::ParseError::getMessage() {
    return *message;
}

bool Method::isChildOf(const google::protobuf::FileDescriptor *fileDescriptor) {
    return fileDescriptor == descriptor->file();
}
