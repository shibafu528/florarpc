#ifndef FLORARPC_IMPORTPROTOSTASK_H
#define FLORARPC_IMPORTPROTOSTASK_H

#include <QList>
#include <QObject>
#include <QProgressDialog>
#include <QThread>
#include <memory>

#include "entity/Protocol.h"

namespace Task {
    class ImportDirectoryWorker;

    class ImportProtosTask : public QObject {
        Q_OBJECT

        Q_DISABLE_COPY(ImportProtosTask)

    public:
        ImportProtosTask(std::vector<std::shared_ptr<Protocol>> &protocols, QStringList &imports,
                         QWidget *parent = nullptr);

        void importDirectoryAsync(const QString &dirname);

    signals:
        void loadFinished(const QList<std::shared_ptr<Protocol>> &protocols, bool hasError);

        void onLogging(const QString &message);

        void finished();

    private slots:
        void onLoadFinished(const QList<std::shared_ptr<Protocol>> &protocols, bool hasError);

        void onProgress(int loaded, int filesCount);

        void onCanceled();

    private:
        std::vector<std::shared_ptr<Protocol>> &protocols;
        QStringList &imports;

        QThread *workerThread;
        QProgressDialog *progressDialog;

        friend ImportDirectoryWorker;
    };
}  // namespace Task

Q_DECLARE_METATYPE(QList<std::shared_ptr<Protocol>>)

#endif  // FLORARPC_IMPORTPROTOSTASK_H
