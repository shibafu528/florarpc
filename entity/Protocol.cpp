#include "Protocol.h"

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <QDataStream>
#include <QDir>
#include <sstream>

#include "util/ProtobufIterator.h"

using namespace google::protobuf::compiler;
using google::protobuf::FileDescriptor;
using google::protobuf::MethodDescriptor;
using google::protobuf::ServiceDescriptor;
using google::protobuf::io::ZeroCopyInputStream;
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

class QFileInputStream : public ZeroCopyInputStream {
public:
    explicit QFileInputStream(unique_ptr<QFile> file)
            : file(move(file)), inputStream(this), inputAdaptor(&inputStream) {}

    ~QFileInputStream() override {
        if (file->isOpen()) {
            file->close();
        }
    }

    bool Next(const void **data, int *size) override {
        return inputAdaptor.Next(data, size);
    }

    void BackUp(int count) override {
        return inputAdaptor.BackUp(count);
    }

    bool Skip(int count) override {
        return inputAdaptor.Skip(count);
    }

    int64_t ByteCount() const override {
        return inputAdaptor.ByteCount();
    }

private:
    class CopyingInputStream : public google::protobuf::io::CopyingInputStream {
    public:
        explicit CopyingInputStream(QFileInputStream *parent) : parent(parent) {}

        int Read(void *buffer, int size) override {
            return parent->file->read((char *) buffer, size);
        }

        int Skip(int count) override {
            return parent->file->skip(count);
        }

    private:
        QFileInputStream *parent;
    };

    unique_ptr<QFile> file;
    CopyingInputStream inputStream;
    google::protobuf::io::CopyingInputStreamAdaptor inputAdaptor;
};

template<class T>
class WellKnownSourceTree : public SourceTree {
    static_assert(std::is_base_of<SourceTree, T>::value == true, "template parameter T must inherit from SourceTree.");
public:
    explicit WellKnownSourceTree(unique_ptr<T> fallback) : fallback(move(fallback)) {}

    ~WellKnownSourceTree() override = default;

    ZeroCopyInputStream *Open(const std::string &filename) override {
        auto file = std::make_unique<QFile>(":/proto/" + QString::fromStdString(filename));
        if (!file->exists()) {
            lastOpenFrom = OpenFrom::Fallback;
            return fallback->Open(filename);
        }

        lastOpenFrom = OpenFrom::Resource;
        if (!file->open(QIODevice::ReadOnly)) {
            return nullptr;
        }

        return new QFileInputStream(move(file));
    }

    std::string GetLastErrorMessage() override {
        if (lastOpenFrom == OpenFrom::Fallback) {
            return fallback->GetLastErrorMessage();
        }
        return SourceTree::GetLastErrorMessage();
    }

    T *getFallback() {
        return fallback.get();
    }

private:
    enum class OpenFrom {
        Resource,
        Fallback
    };

    unique_ptr<T> fallback;
    OpenFrom lastOpenFrom = OpenFrom::Resource;
};

Protocol::Protocol(const QFileInfo &file, const QStringList &imports) : source(file) {
    auto errorCollectorStub = std::make_unique<ErrorCollectorStub>();
    auto wellKnownSourceTree = std::make_unique<WellKnownSourceTree<DiskSourceTree>>(
            std::make_unique<DiskSourceTree>());
    importer = std::make_unique<Importer>(wellKnownSourceTree.get(), errorCollectorStub.get());

    wellKnownSourceTree->getFallback()->MapPath("", file.dir().absolutePath().toStdString());
    for (const QString &include : imports) {
        wellKnownSourceTree->getFallback()->MapPath("", include.toStdString());
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
