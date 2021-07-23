#ifndef FLORARPC_WELLKNOWNSOURCETREE_H
#define FLORARPC_WELLKNOWNSOURCETREE_H

#include <google/protobuf/compiler/importer.h>

#include <QFile>

#include "QFileInputStream.h"

namespace importer {
    /**
     * Well-known typesの定義をQtリソースから読み込むためのSourceTree
     */
    template <class T>
    class WellKnownSourceTree : public google::protobuf::compiler::SourceTree {
        static_assert(std::is_base_of<SourceTree, T>::value == true,
                      "template parameter T must inherit from SourceTree.");

    public:
        explicit WellKnownSourceTree(std::unique_ptr<T> fallback) : fallback(move(fallback)) {}

        ~WellKnownSourceTree() override = default;

        google::protobuf::io::ZeroCopyInputStream *Open(const std::string &filename) override {
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

        T *getFallback() { return fallback.get(); }

    private:
        enum class OpenFrom { Resource, Fallback };

        std::unique_ptr<T> fallback;
        OpenFrom lastOpenFrom = OpenFrom::Resource;
    };
}  // namespace importer

#endif  // FLORARPC_WELLKNOWNSOURCETREE_H
