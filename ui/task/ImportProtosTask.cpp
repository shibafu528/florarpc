#include "ImportProtosTask.h"

#include <QDebug>
#include <QDirIterator>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QRunnable>
#include <QThreadPool>
#include <QWriteLocker>

namespace Task {
    class ImportDirectoryWorker : public QObject, public QRunnable {
        Q_OBJECT

    public:
        explicit ImportDirectoryWorker(const ImportProtosTask &task, const QString &dirname)
            : task(task), dirname(dirname), lock(), interrupted(false) {}

        void run() override {
            runInternal();
            emit finished();
        }

        void interrupt() {
            QWriteLocker locker(&lock);
            interrupted = true;
        }

    signals:
        void loadFinished(const QList<std::shared_ptr<Protocol>> &protocols, bool hasError);

        void onProgress(int loaded, int filesCount);

        void onLogging(const QString &message);

        void finished();

    private:
        const ImportProtosTask &task;
        const QString dirname;
        QReadWriteLock lock;
        bool interrupted;

        bool isInterrupted() {
            QReadLocker locker(&lock);
            return interrupted;
        }

        void runInternal() {
            QStringList filenames;
            QDirIterator iterator(dirname, QStringList() << "*.proto", QDir::Files, QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                if (isInterrupted()) {
                    qDebug() << "ImportDirectoryWorker interrupted!";
                    return;
                }
                filenames << iterator.next();
            }
            emit onProgress(0, filenames.size());

            QList<std::shared_ptr<Protocol>> successes;
            bool error = false;
            uint64_t done = 0;
            for (const auto &filename : filenames) {
                if (isInterrupted()) {
                    qDebug() << "ImportDirectoryWorker interrupted!";
                    return;
                }

                QFileInfo file(filename);

                if (std::any_of(task.protocols.begin(), task.protocols.end(),
                                [file](std::shared_ptr<Protocol> &p) { return p->getSource() == file; })) {
                    emit onProgress(++done, filenames.size());
                    continue;
                }

                try {
                    const auto protocol = std::make_shared<Protocol>(file, task.imports);
                    successes.push_back(protocol);
                } catch (ProtocolLoadException &e) {
                    emit onLogging(QString("Protoファイルの読込中にエラー: %1").arg(filename));

                    for (const auto &err : *e.errors) {
                        emit onLogging(QString::fromStdString(err));
                    }

                    error = true;
                }

                emit onProgress(++done, filenames.size());
            }

            emit loadFinished(successes, error);
        }
    };
}  // namespace Task

Task::ImportProtosTask::ImportProtosTask(std::vector<std::shared_ptr<Protocol>> &protocols, QStringList &imports,
                                         QWidget *parent)
    : QObject(parent), protocols(protocols), imports(imports), worker(nullptr), progressDialog(nullptr) {
    qRegisterMetaType<QList<std::shared_ptr<Protocol>>>();
}

Task::ImportProtosTask::~ImportProtosTask() {}

void Task::ImportProtosTask::importDirectoryAsync(const QString &dirname) {
    worker = new ImportDirectoryWorker(*this, dirname);
    connect(worker, &ImportDirectoryWorker::loadFinished, this, &ImportProtosTask::onLoadFinished);
    connect(worker, &ImportDirectoryWorker::loadFinished, this, &ImportProtosTask::loadFinished);
    connect(worker, &ImportDirectoryWorker::onProgress, this, &ImportProtosTask::onProgress);
    connect(worker, &ImportDirectoryWorker::onLogging, this, &ImportProtosTask::onLogging);
    connect(worker, &ImportDirectoryWorker::finished, this, &ImportProtosTask::finished);

    progressUpdateThrottle = new QTimer(this);
    progressUpdateThrottle->setSingleShot(true);
    connect(progressUpdateThrottle, &QTimer::timeout, this, &ImportProtosTask::onThrottledProgress);

    progressDialog = new QProgressDialog("読み込み中...", "キャンセル", 0, 0, qobject_cast<QWidget *>(parent()),
                                         Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::Sheet);
    progressDialog->setWindowModality(Qt::WindowModal);
    connect(progressDialog, &QProgressDialog::canceled, this, &ImportProtosTask::onCanceled);
    connect(worker, &ImportDirectoryWorker::finished, progressDialog, &QProgressDialog::close);

    progressDialog->show();
    QThreadPool::globalInstance()->start(worker);
}

void Task::ImportProtosTask::onProgress(int loaded, int filesCount) {
    throttledLoaded = loaded;
    throttledFilesCount = filesCount;
    if (!progressUpdateThrottle->isActive()) {
        progressUpdateThrottle->start(std::chrono::milliseconds(500));
        onThrottledProgress();
    }
}

void Task::ImportProtosTask::onThrottledProgress() {
    if (throttledLoaded == -1) {
        return;
    }
    if (progressDialog != nullptr) {
        progressDialog->setMaximum(throttledFilesCount);
        progressDialog->setValue(throttledLoaded);
        throttledFilesCount = -1;
        throttledLoaded = -1;
    }
}

void Task::ImportProtosTask::onLoadFinished(const QList<std::shared_ptr<Protocol>> &protocols, bool hasError) {
    if (progressUpdateThrottle->isActive()) {
        progressUpdateThrottle->stop();
        onThrottledProgress();
    }
}

void Task::ImportProtosTask::onCanceled() {
    if (worker != nullptr) {
        worker->interrupt();
    }
}

#include "ImportProtosTask.moc"
