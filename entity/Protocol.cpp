#include "Protocol.h"

#include <QDataStream>
#include <QDir>
#include <sstream>

#include "util/ProtobufIterator.h"
#include "util/importer/FloraSourceTree.h"
#include "util/importer/WellKnownSourceTree.h"

using namespace importer;
using namespace google::protobuf::compiler;
using google::protobuf::FileDescriptor;
using google::protobuf::MethodDescriptor;
using google::protobuf::ServiceDescriptor;
using std::move;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

class ErrorCollectorStub : public MultiFileErrorCollector {
public:
    unique_ptr<vector<std::string>> errors = std::make_unique<vector<std::string>>();

    void AddError(const std::string &filename, int line, int column, const std::string &message) override {
        std::stringstream error;
        error << filename << " " << line << ":" << column << " - " << message;
        errors->push_back(error.str());
    }
};

Protocol::Protocol(const QFileInfo &file, const QStringList &imports) : source(file) {
    auto errorCollectorStub = std::make_unique<ErrorCollectorStub>();
    auto wellKnownSourceTree =
        std::make_unique<WellKnownSourceTree<FloraSourceTree>>(std::make_unique<FloraSourceTree>());
    importer = std::make_unique<Importer>(wellKnownSourceTree.get(), errorCollectorStub.get());

    wellKnownSourceTree->getFallback()->map("", file.dir().absolutePath().toStdString());
    for (const QString &include : imports) {
        wellKnownSourceTree->getFallback()->map("", include.toStdString());
    }
    auto fd = importer->Import(file.fileName().toStdString());
    if (fd == nullptr) {
        throw ProtocolLoadException(move(errorCollectorStub->errors));
    }

    if (fd->service_count() == 0) {
        throw ServiceNotFoundException();
    }

    errorCollector = move(errorCollectorStub);
    sourceTree = move(wellKnownSourceTree);
    fileDescriptor = fd;
}

const google::protobuf::MethodDescriptor *Protocol::findMethodByRef(const florarpc::MethodRef &ref) {
    ProtobufIterator::Iterable<const FileDescriptor, const ServiceDescriptor> services(fileDescriptor);
    for (const auto &service : services) {
        if (service->full_name() != ref.service_name()) {
            continue;
        }

        ProtobufIterator::Iterable<const ServiceDescriptor, const MethodDescriptor> methods(service);
        for (const auto &method : methods) {
            if (method->name() == ref.method_name()) {
                return method;
            }
        }
    }

    return nullptr;
}

ProtocolLoadException::ProtocolLoadException(std::unique_ptr<std::vector<std::string>> errors)
    : std::exception(), errors(move(errors)) {}
