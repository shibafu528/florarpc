#include "Protocol.h"
#include <google/protobuf/compiler/importer.h>
#include <sstream>
#include <QDir>

using namespace google::protobuf::compiler;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::move;

class ErrorCollectorStub : public MultiFileErrorCollector {
public:
    unique_ptr<vector<std::string>> errors = std::make_unique<vector<std::string>>();

    void AddError(const std::string &filename, int line, int column, const std::string &message) override {
        std::stringstream error;
        error << filename << " " << line << ":" << column << " - " << message;
        errors->push_back(error.str());
    }
};

Protocol::Protocol(DiskSourceTree *sourceTree,
                   Importer *importer,
                   MultiFileErrorCollector *errorCollector,
                   const google::protobuf::FileDescriptor *fd)
        : sourceTree(sourceTree), importer(importer), errorCollector(errorCollector), fd(fd) {}

unique_ptr<Protocol> Protocol::loadFromFile(QFileInfo &file) {
    auto errorCollector = new ErrorCollectorStub();
    auto sourceTree = new DiskSourceTree();
    auto importer = new Importer(sourceTree, errorCollector);

    sourceTree->MapPath("", file.dir().absolutePath().toStdString());
    auto fd = importer->Import(file.fileName().toStdString());
    if (fd == nullptr) {
        throw ProtocolLoadException(move(errorCollector->errors));
    }

    return unique_ptr<Protocol>(new Protocol(sourceTree, importer, errorCollector, fd));
}

ProtocolLoadException::ProtocolLoadException(std::unique_ptr<std::vector<std::string>> errors)
        : std::exception(), errors(move(errors)) {}
