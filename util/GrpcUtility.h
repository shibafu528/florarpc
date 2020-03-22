#ifndef FLORARPC_GRPCUTILITY_H
#define FLORARPC_GRPCUTILITY_H

#include <google/protobuf/message.h>
#include <grpcpp/support/byte_buffer.h>
#include <QString>

namespace GrpcUtility {
    std::unique_ptr<grpc::ByteBuffer> serializeMessage(const google::protobuf::Message &message);
    bool parseMessage(const grpc::ByteBuffer &buffer, google::protobuf::Message &message);
    QString errorCodeToString(grpc::StatusCode statusCode);
}

#endif //FLORARPC_GRPCUTILITY_H
