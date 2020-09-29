#include "ImportProtosTask.h"

#include <QDebug>
#include <QDirIterator>

namespace Task {
    class ImportDirectoryWorker : public QObject {
        Q_OBJECT

    public:
        explicit ImportDirectoryWorker(const ImportProtosTask &task, const QString &dirname)
            : task(task), dirname(dirname) {}

    signals:
        void loadFinished(const QList<std::shared_ptr<Protocol>> &protocols, bool hasError);

        void onProgress(int loaded, int filesCount);

        void onLogging(const QString &message);

        void finished();

    public slots:
        void doWork() {
            doWorkInternal();
            emit finished();
        }

    private:
        const ImportProtosTask &task;
        const QString dirname;

        void doWorkInternal() {
            QStringList filenames;
            QDirIterator iterator(dirname, QStringList() << "*.proto", QDir::Files, QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                if (QThread::currentThread()->isInterruptionRequested()) {
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
                if (QThread::currentThread()->isInterruptionRequested()) {
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
    : QObject(parent), protocols(protocols), imports(imports), workerThread(nullptr), progressDialog(nullptr) {
    qRegisterMetaType<QList<std::shared_ptr<Protocol>>>();
}

Task::ImportProtosTask::~ImportProtosTask() {
    if (workerThread != nullptr) {
        workerThread->quit();
        workerThread->wait();
    }
}

void Task::ImportProtosTask::importDirectoryAsync(const QString &dirname) {
    workerThread = new QThread(this);
    workerThread->setObjectName("Task::ImportProtosTask worker");
    connect(workerThread, &QThread::finished, this, &ImportProtosTask::finished);

    auto worker = new ImportDirectoryWorker(*this, dirname);
    worker->moveToThread(workerThread);
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::started, worker, &ImportDirectoryWorker::doWork);
    connect(worker, &ImportDirectoryWorker::loadFinished, this, &ImportProtosTask::loadFinished);
    connect(worker, &ImportDirectoryWorker::onProgress, this, &ImportProtosTask::onProgress);
    connect(worker, &ImportDirectoryWorker::onLogging, this, &ImportProtosTask::onLogging);
    connect(worker, &ImportDirectoryWorker::finished, workerThread, &QThread::quit);

    progressUpdateThrottle = new QTimer(this);
    progressUpdateThrottle->setSingleShot(true);
    connect(progressUpdateThrottle, &QTimer::timeout, this, &ImportProtosTask::onThrottledProgress);

    progressDialog = new QProgressDialog("読み込み中...", "キャンセル", 0, 0, qobject_cast<QWidget *>(parent()),
                                         Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::Sheet);
    progressDialog->setWindowModality(Qt::WindowModal);
    connect(progressDialog, &QProgressDialog::canceled, this, &ImportProtosTask::onCanceled);
    connect(workerThread, &QThread::finished, progressDialog, &QProgressDialog::close);

    progressDialog->show();
    workerThread->start();
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

void Task::ImportProtosTask::onCanceled() { workerThread->requestInterruption(); }

#include "ImportProtosTask.moc"
