#ifndef FLORARPC_METHOD_H
#define FLORARPC_METHOD_H

#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <grpcpp/support/byte_buffer.h>

class Method {
public:
    class ParseError : public std::exception {
    public:
        explicit ParseError(std::unique_ptr<std::string> message);
        const std::string& getMessage();

    private:
        std::unique_ptr<std::string> message;
    };

    explicit Method(const google::protobuf::MethodDescriptor *descriptor);
    const std::string& getFullName();
    std::string getRequestPath();
    std::string makeRequestSkeleton();
    std::unique_ptr<google::protobuf::Message> parseRequest(google::protobuf::DynamicMessageFactory &factory, const std::string &json);
    std::unique_ptr<google::protobuf::Message> parseResponse(google::protobuf::DynamicMessageFactory &factory, grpc::ByteBuffer &buffer);

private:
    const google::protobuf::MethodDescriptor *descriptor;
};

#endif //FLORARPC_METHOD_H
