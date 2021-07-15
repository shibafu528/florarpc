#include "Method.h"

#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>

#include <sstream>

#include "../util/GrpcUtility.h"
#include "../util/ProtobufJsonPrinter.h"
#include "google/rpc/status.pb.h"

Method::Method(std::shared_ptr<Protocol> protocol, const google::protobuf::MethodDescriptor *descriptor)
    : protocol(std::move(protocol)), descriptor(descriptor) {}

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
        throw ParseError(std::make_unique<std::string>(parseStatus.message()));
    }
    return reqMessage;
}

std::unique_ptr<google::protobuf::Message> Method::parseResponse(google::protobuf::DynamicMessageFactory &factory,
                                                                 const grpc::ByteBuffer &buffer) {
    auto resProto = factory.GetPrototype(descriptor->output_type());
    auto resMessage = std::unique_ptr<google::protobuf::Message>(resProto->New());
    GrpcUtility::parseMessage(buffer, *resMessage);
    return resMessage;
}

std::unique_ptr<google::protobuf::Message> Method::parseErrorDetails(google::protobuf::DynamicMessageFactory &factory,
                                                                     const std::string &buffer) {
    auto pool = protocol->getFileDescriptor()->pool();
    // import proto file
    {
        pool->FindFileByName("google/rpc/status.proto");
        pool->FindFileByName("google/rpc/error_details.proto");
        pool->FindFileByName("google/rpc/code.proto");
    }
    auto desc = pool->FindMessageTypeByName("google.rpc.Status");
    if (desc != nullptr) {
        // parse message using method's descriptor pool
        auto proto = factory.GetPrototype(desc);
        auto message = std::unique_ptr<google::protobuf::Message>(proto->New());
        if (message->ParseFromString(buffer)) {
            return message;
        } else {
            return nullptr;
        }
    } else {
        // fallback, retry with default descriptor pool
        auto message = std::make_unique<google::rpc::Status>();
        if (message->ParseFromString(buffer)) {
            return message;
        } else {
            return nullptr;
        }
    }
}

void Method::writeMethodRef(florarpc::MethodRef &ref) {
    ref.set_service_name(descriptor->service()->full_name());
    ref.set_method_name(descriptor->name());
    ref.set_file_name(protocol->getSourceAbsolutePath());
}

void Method::exportTo(florarpc::DescriptorExports &dest) const {
    descriptor->input_type()->CopyTo(dest.mutable_request());

    descriptor->input_type()->file()->CopyTo(dest.mutable_request_owner_file());
    descriptor->input_type()->file()->CopyJsonNameTo(dest.mutable_request_owner_file());

    descriptor->output_type()->CopyTo(dest.mutable_response());

    descriptor->output_type()->file()->CopyTo(dest.mutable_response_owner_file());
    descriptor->output_type()->file()->CopyJsonNameTo(dest.mutable_response_owner_file());

    descriptor->CopyTo(dest.mutable_method());

    descriptor->service()->CopyTo(dest.mutable_method_owner_service());

    descriptor->file()->CopyTo(dest.mutable_method_owner_file());
    descriptor->file()->CopyJsonNameTo(dest.mutable_method_owner_file());
}

Method::ParseError::ParseError(std::unique_ptr<std::string> message) : message(std::move(message)) {}

const std::string &Method::ParseError::getMessage() { return *message; }

bool Method::isChildOf(const google::protobuf::FileDescriptor *fileDescriptor) const {
    return fileDescriptor == descriptor->file();
}
