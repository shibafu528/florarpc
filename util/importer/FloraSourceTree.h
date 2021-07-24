#ifndef FLORARPC_FLORASOURCETREE_H
#define FLORARPC_FLORASOURCETREE_H

#include <google/protobuf/compiler/importer.h>

namespace importer {
    /**
     * DiskSourceTreeのI/OをQtのAPIに差し替えたバージョン
     */
    class FloraSourceTree : public google::protobuf::compiler::SourceTree {
    public:
        google::protobuf::io::ZeroCopyInputStream *Open(const std::string &filename) override;

        std::string GetLastErrorMessage() override;

        void map(const std::string &virtualPath, const std::string &realPath);

    private:
        std::vector<std::pair<std::string, std::string>> mappings;
        std::string lastErrorMessage;

        google::protobuf::io::ZeroCopyInputStream *openFile(const std::string &filename);
    };
}  // namespace importer

#endif  // FLORARPC_FLORASOURCETREE_H
