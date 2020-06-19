#include "GrpcUtility.h"

std::unique_ptr<grpc::ByteBuffer> GrpcUtility::serializeMessage(const google::protobuf::Message &message) {
    std::string buffer;
    message.SerializeToString(&buffer);
    auto slice = grpc::Slice(buffer);
    return std::make_unique<grpc::ByteBuffer>(&slice, 1);
}

bool GrpcUtility::parseMessage(const grpc::ByteBuffer &buffer, google::protobuf::Message &message) {
    std::vector<grpc::Slice> slices;
    buffer.Dump(&slices);
    std::string buf;
    buf.reserve(buffer.Length());
    for (const auto &s : slices) {
        buf.append(reinterpret_cast<const char *>(s.begin()), s.size());
    }
    return message.ParseFromString(buf);
}

QString GrpcUtility::errorCodeToString(grpc::StatusCode statusCode) {
    QString code;
    switch (statusCode) {
        case grpc::OK:
            code = "OK";
            break;
        case grpc::CANCELLED:
            code = "CANCELLED";
            break;
        case grpc::UNKNOWN:
            code = "UNKNOWN";
            break;
        case grpc::INVALID_ARGUMENT:
            code = "INVALID_ARGUMENT";
            break;
        case grpc::DEADLINE_EXCEEDED:
            code = "DEADLINE_EXCEEDED";
            break;
        case grpc::NOT_FOUND:
            code = "NOT_FOUND";
            break;
        case grpc::ALREADY_EXISTS:
            code = "ALREADY_EXISTS";
            break;
        case grpc::PERMISSION_DENIED:
            code = "PERMISSION_DENIED";
            break;
        case grpc::UNAUTHENTICATED:
            code = "UNAUTHENTICATED";
            break;
        case grpc::RESOURCE_EXHAUSTED:
            code = "RESOURCE_EXHAUSTED";
            break;
        case grpc::FAILED_PRECONDITION:
            code = "FAILED_PRECONDITION";
            break;
        case grpc::ABORTED:
            code = "ABORTED";
            break;
        case grpc::OUT_OF_RANGE:
            code = "OUT_OF_RANGE";
            break;
        case grpc::UNIMPLEMENTED:
            code = "UNIMPLEMENTED";
            break;
        case grpc::INTERNAL:
            code = "INTERNAL";
            break;
        case grpc::UNAVAILABLE:
            code = "UNAVAILABLE";
            break;
        case grpc::DATA_LOSS:
            code = "DATA_LOSS";
            break;
        default:
            code = "?";
            break;
    }
    code += QString::asprintf(" (%d)", statusCode);
    return code;
}
