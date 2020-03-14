#ifndef FLORARPC_PROTOCOL_H
#define FLORARPC_PROTOCOL_H

#include <QFileInfo>
#include <memory>
#include <google/protobuf/compiler/importer.h>

class Protocol {
public:
    Protocol(QFileInfo &file, QStringList &imports);
    inline const google::protobuf::FileDescriptor* getFileDescriptor() const { return fileDescriptor; };

private:
    std::unique_ptr<google::protobuf::compiler::SourceTree> sourceTree;
    std::unique_ptr<google::protobuf::compiler::Importer> importer;
    std::unique_ptr<google::protobuf::compiler::MultiFileErrorCollector> errorCollector;
    const google::protobuf::FileDescriptor *fileDescriptor;
};

class ProtocolLoadException : public std::exception {
public:
    explicit ProtocolLoadException(std::unique_ptr<std::vector<std::string>> errors);

    const std::unique_ptr<std::vector<std::string>> errors;
};

#endif //FLORARPC_PROTOCOL_H
