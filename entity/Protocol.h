#ifndef FLORARPC_PROTOCOL_H
#define FLORARPC_PROTOCOL_H

#include <google/protobuf/compiler/importer.h>

#include <QFileInfo>
#include <memory>

#include "florarpc/workspace.pb.h"

class Protocol {
public:
    Protocol(const QFileInfo &file, const QStringList &imports);

    inline const QFileInfo &getSource() const { return source; }

    inline std::string getSourceAbsolutePath() const { return source.absoluteFilePath().toStdString(); }

    inline const google::protobuf::FileDescriptor *getFileDescriptor() const { return fileDescriptor; };

    const google::protobuf::MethodDescriptor *findMethodByRef(const florarpc::MethodRef &ref);

private:
    const QFileInfo source;
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
