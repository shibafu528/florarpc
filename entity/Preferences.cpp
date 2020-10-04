#include "Preferences.h"

#include <QFile>
#include <QReadLocker>
#include <QSaveFile>
#include <QWriteLocker>

#include "flora_constants.h"

Preferences::Preferences(const QString &filePath)
    : filePath(filePath), data(), lock(), latestSaveResult(SaveResult::Success) {}

Preferences::LoadResult Preferences::load() {
    QFile file(filePath);
    if (!file.exists()) {
        // ファイルが存在しない場合は、空の設定がロードされたことと等価
        return LoadResult::Success;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return LoadResult::CantOpen;
    }

    const auto bin = file.readAll();
    file.close();

    florarpc::Preferences newPref;
    bool success = newPref.ParseFromString(bin.toStdString());
    if (!success) {
        return LoadResult::ParseError;
    }

    florarpc::Version *version = newPref.mutable_app_version();
    version->set_major(FLORA_VERSION_MAJOR);
    version->set_minor(FLORA_VERSION_MINOR);
    version->set_patch(FLORA_VERSION_PATCH);
    version->set_tweak(FLORA_VERSION_TWEAK);

    data.CopyFrom(newPref);
    return LoadResult::Success;
}

Preferences::SaveResult Preferences::save() {
    std::string bin;
    if (!data.SerializeToString(&bin)) {
        return latestSaveResult = SaveResult::SerializeError;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return latestSaveResult = SaveResult::CantOpen;
    }

    file.write(QByteArray::fromStdString(bin));
    if (!file.commit()) {
        return latestSaveResult = SaveResult::WriteError;
    }

    return latestSaveResult = Preferences::SaveResult::Success;
}

void Preferences::read(const std::function<void(const florarpc::Preferences &)> &reader) {
    QReadLocker locker(&lock);
    reader(data);
}

void Preferences::mutation(const std::function<void(florarpc::Preferences &)> &mutator) {
    QWriteLocker locker(&lock);
    mutator(data);
    // TODO: delayed save, notify error
    save();
}
