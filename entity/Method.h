#ifndef FLORARPC_METHOD_H
#define FLORARPC_METHOD_H

#include "florarpc/workspace.pb.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <grpcpp/support/byte_buffer.h>

class Method {
public:
    class ParseError : public std::exception {
    public:
        explicit ParseError(std::unique_ptr<std::string> message);

        const std::string &getMessage();

    private:
        std::unique_ptr<std::string> message;
    };

    explicit Method(const google::protobuf::MethodDescriptor *descriptor);

    const std::string &getFullName() const;

    std::string getRequestPath() const;

    inline bool isClientStreaming() const {
        return descriptor->client_streaming();
    }

    inline bool isServerStreaming() const {
        return descriptor->server_streaming();
    }

    std::string makeRequestSkeleton();

    std::unique_ptr<google::protobuf::Message>
    parseRequest(google::protobuf::DynamicMessageFactory &factory, const std::string &json);

    std::unique_ptr<google::protobuf::Message>
    parseResponse(google::protobuf::DynamicMessageFactory &factory, const grpc::ByteBuffer &buffer);

    void writeMethodRef(florarpc::MethodRef &ref);

private:
    const google::protobuf::MethodDescriptor *descriptor;
};

#endif //FLORARPC_METHOD_H
