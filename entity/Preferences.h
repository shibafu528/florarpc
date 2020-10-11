#ifndef FLORARPC_PREFERENCES_H
#define FLORARPC_PREFERENCES_H

#include <QReadWriteLock>
#include <QString>
#include <functional>

#include "florarpc/preferences.pb.h"

class Preferences {
public:
    enum class LoadResult {
        Success,
        CantOpen,
        ParseError,
    };

    enum class SaveResult {
        Success,
        CantOpen,
        SerializeError,
        WriteError,
    };

    explicit Preferences(const QString &filePath);

    LoadResult load();

    SaveResult save();

    inline SaveResult getLatestSaveResult() const { return latestSaveResult; }

    void read(const std::function<void(const florarpc::Preferences &)> &reader);

    template <typename T>
    inline T read(const std::function<T(const florarpc::Preferences &)> &reader) {
        QReadLocker locker(&lock);
        return reader(data);
    }

    void mutation(const std::function<void(florarpc::Preferences &)> &mutator);

    void addRecentWorkspace(const QString &file);

private:
    QString filePath;
    florarpc::Preferences data;
    QReadWriteLock lock;
    SaveResult latestSaveResult;
};

// impl in main.cpp
Preferences &sharedPref();

#endif  // FLORARPC_PREFERENCES_H
