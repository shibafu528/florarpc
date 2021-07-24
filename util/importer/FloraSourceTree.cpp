#include "FloraSourceTree.h"

#include <absl/strings/match.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_split.h>

#include <QDir>

#include "QFileInputStream.h"

static std::string canonicalize(std::string path) {
    bool isUNC = false;
    if (absl::StartsWith(path, "//")) {
        isUNC = true;
    }

    std::vector<std::string> canonicalParts;
    std::vector<std::string> parts = absl::StrSplit(path, "/", absl::SkipEmpty());
    for (const auto& part : parts) {
        if (part != ".") {
            canonicalParts.push_back(part);
        }
    }

    std::string result = absl::StrJoin(canonicalParts, "/");
    if (!path.empty() && path[0] == '/') {
        if (isUNC) {
            result = "//" + result;
        } else {
            result = '/' + result;
        }
    }
    if (absl::EndsWith(path, "/") && !absl::EndsWith(result, "/")) {
        result += '/';
    }

    return result;
}

static bool contains_parent_reference(const std::string& path) {
    return path == ".." || absl::StartsWith(path, "../") || absl::EndsWith(path, "/..") ||
           absl::StrContains(path, "/../");
}

static bool is_windows_absolute_path(const std::string& path) {
#ifdef _WIN32
    // UNC path (//host)
    if (path.size() >= 2 && path[0] == '/' && path[1] == '/') {
        return true;
    }
    // Starts with drive letter (C:/foo)
    return path.size() >= 3 && path[1] == ':' && isalpha(path[0]) && path[2] == '/' && path.find_last_of(':') == 1;
#else
    return false;
#endif
}

static bool apply_mapping(const std::string& filename, const std::string& oldPrefix, const std::string& newPrefix,
                          std::string* result) {
    if (oldPrefix.empty()) {
        if (contains_parent_reference(filename)) {
            return false;
        }
        if (absl::StartsWith(filename, "/") || is_windows_absolute_path(filename)) {
            return false;
        }
        result->assign(newPrefix);
        if (!result->empty()) {
            result->push_back('/');
        }
        result->append(filename);
        return true;
    } else if (absl::StartsWith(filename, oldPrefix)) {
        if (filename.size() == oldPrefix.size()) {
            *result = newPrefix;
            return true;
        }

        int afterPrefixStart = -1;
        if (filename[oldPrefix.size()] == '/') {
            afterPrefixStart = oldPrefix.size() + 1;
        } else if (filename[oldPrefix.size() - 1] == '/') {
            afterPrefixStart = oldPrefix.size();
        }
        if (afterPrefixStart != -1) {
            const auto afterPrefix = filename.substr(afterPrefixStart);
            if (contains_parent_reference(afterPrefix)) {
                return false;
            }
            result->assign(newPrefix);
            if (!result->empty()) {
                result->push_back('/');
            }
            result->append(afterPrefix);
            return true;
        }
    }
}

google::protobuf::io::ZeroCopyInputStream* importer::FloraSourceTree::Open(const std::string& filename) {
    if (filename != canonicalize(filename) || contains_parent_reference(filename)) {
        lastErrorMessage = R"(Backslashes, consecutive slashes, ".", or ".." are not allowed in the virtual path)";
        return nullptr;
    }

    for (const auto& pair : mappings) {
        std::string realpath;
        if (apply_mapping(filename, pair.first, pair.second, &realpath)) {
            auto stream = openFile(realpath);
            if (stream != nullptr) {
                return stream;
            }
        }
    }

    lastErrorMessage = "File not found.";
    return nullptr;
}

std::string importer::FloraSourceTree::GetLastErrorMessage() { return lastErrorMessage; }

void importer::FloraSourceTree::map(const std::string& virtualPath, const std::string& realPath) {
    mappings.emplace_back(virtualPath, realPath);
}

google::protobuf::io::ZeroCopyInputStream* importer::FloraSourceTree::openFile(const std::string& filename) {
    const auto qFilename = QString::fromStdString(filename);
    if (QDir(qFilename).exists()) {
        lastErrorMessage = "Input file is a directory.";
        return nullptr;
    }
    auto file = std::make_unique<QFile>(qFilename);
    if (!file->open(QIODevice::ReadOnly)) {
        lastErrorMessage = QString("%1: %2").arg(file->errorString(), qFilename).toStdString();
        return nullptr;
    }
    return new QFileInputStream(move(file));
}
