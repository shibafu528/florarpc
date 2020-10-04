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

    explicit Preferences(QString &filePath);

    LoadResult load();

    SaveResult save();

    void read(const std::function<void(const florarpc::Preferences &)> &reader);

    void mutation(const std::function<void(florarpc::Preferences &)> &mutator);

private:
    QString filePath;
    florarpc::Preferences data;
    QReadWriteLock lock;
    SaveResult latestSaveResult;
};

// impl in main.cpp
Preferences &sharedPref();

#endif  // FLORARPC_PREFERENCES_H
