#ifndef FLORARPC_PROTOCOL_H
#define FLORARPC_PROTOCOL_H

#include <QFileInfo>
#include <memory>
#include <google/protobuf/compiler/importer.h>

class Protocol;

class Protocol {
public:
    static std::unique_ptr<Protocol> loadFromFile(QFileInfo &file, QStringList &imports);
    inline const google::protobuf::FileDescriptor* getFileDescriptor() { return fd; };

private:
    Protocol(google::protobuf::compiler::SourceTree *sourceTree,
             google::protobuf::compiler::Importer *importer,
             google::protobuf::compiler::MultiFileErrorCollector *errorCollector,
             const google::protobuf::FileDescriptor *fd);

    std::unique_ptr<google::protobuf::compiler::SourceTree> sourceTree;
    std::unique_ptr<google::protobuf::compiler::Importer> importer;
    std::unique_ptr<google::protobuf::compiler::MultiFileErrorCollector> errorCollector;
    const google::protobuf::FileDescriptor *fd;
};

class ProtocolLoadException : public std::exception {
public:
    explicit ProtocolLoadException(std::unique_ptr<std::vector<std::string>> errors);

    const std::unique_ptr<std::vector<std::string>> errors;
};

#endif //FLORARPC_PROTOCOL_H
