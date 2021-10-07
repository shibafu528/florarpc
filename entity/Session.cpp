#include "Session.h"

#include <grpcpp/create_channel.h>
#include <grpcpp/generic/generic_stub.h>

#include <QDebug>
#include <QRunnable>

class Session::QueueWatcher : public QObject {
    Q_OBJECT

public:
    explicit QueueWatcher(Session &session) : session(session) {}

signals:

    void messageSent();

    void messageReceived(const grpc::ByteBuffer &buffer);

    void initialMetadataReceived(const Session::Metadata &metadata);

    void trailingMetadataReceived(const Session::Metadata &metadata);

    void finished(int code, const QString &message, const QByteArray &details);

    void aborted();

    void finish();

public slots:

    void doWork() {
        using std::chrono::system_clock;

        void *gotTag;
        bool ok;
        while (auto event = session.queue.AsyncNext(&gotTag, &ok, system_clock::now())) {
            if (event == grpc::CompletionQueue::SHUTDOWN) {
                break;
            }
            if (event == grpc::CompletionQueue::TIMEOUT) {
                if (QThread::currentThread()->isInterruptionRequested()) {
                    break;
                }
                continue;
            }

            if (!ok) {
                emit finish();
                continue;
            }

            switch (session.sequence) {
                case Sequence::Preparing:
                    onSuccessStartCall();
                    break;
                case Sequence::Connected:
                    if (gotTag == session.readTag()) {
                        onSuccessRead();
                    } else if (gotTag == session.writeTag()) {
                        onSuccessWrite();
                    } else {
                        qDebug() << "Unknown tag!! : " << gotTag;
                    }
                    break;
                case Sequence::WritesDone:
                    if (gotTag == session.readTag()) {
                        onSuccessRead();
                    } else if (gotTag == session.writeTag()) {
                        onSuccessWritesDone();
                    } else {
                        qDebug() << "Unknown tag!! : " << gotTag;
                    }
                    break;
                case Sequence::Finishing:
                    onSuccessFinish();
                    return;
            }
        }

        // Illegal sequence
        qDebug() << "Session Abort!!";
        emit aborted();
    }

private:
    Session &session;

    void onSuccessStartCall() {
        qDebug() << __FUNCTION__;
        session.sequence = Sequence::Connected;
        session.writeTag.advance();
        if (session.method.isClientStreaming()) {
            session.call->Write(session.writeBuffer, session.writeTag());
        } else {
            session.call->WriteLast(session.writeBuffer, grpc::WriteOptions(), session.writeTag());
        }
        session.call->Read(&session.readBuffer, session.readTag());
    }

    void onSuccessRead() {
        qDebug() << __FUNCTION__;
        if (!session.receivedInitialMetadata) {
            session.receivedInitialMetadata = true;
            Metadata metadata;
            for (const auto &[key, value] : session.context.GetServerInitialMetadata()) {
                metadata.insert(QString::fromLatin1(key.data(), key.size()),
                                QString::fromLatin1(value.data(), value.size()));
            }
            emit initialMetadataReceived(metadata);
        }

        grpc::ByteBuffer buffer(session.readBuffer);
        emit messageReceived(buffer);

        session.readTag.advance();
        session.readBuffer.Clear();
        session.call->Read(&session.readBuffer, session.readTag());
    }

    void onSuccessWrite() {
        qDebug() << __FUNCTION__;
        session.writeBuffer.Clear();
        emit messageSent();
    }

    void onSuccessWritesDone() { qDebug() << __FUNCTION__; }

    void onSuccessFinish() {
        qDebug() << __FUNCTION__;
        session.endTime = std::chrono::steady_clock::now();
        Metadata metadata;
        for (const auto &[key, value] : session.context.GetServerTrailingMetadata()) {
            metadata.insert(QString::fromLatin1(key.data(), key.size()),
                            QString::fromLatin1(value.data(), value.size()));
        }
        emit trailingMetadataReceived(metadata);
        emit finished(session.statusBuffer.error_code(), QString::fromStdString(session.statusBuffer.error_message()),
                      QByteArray::fromStdString(session.statusBuffer.error_details()));
    }
};

Session::Session(const Method &method, const QString &serverAddress, std::shared_ptr<grpc::ChannelCredentials> &creds,
                 const grpc::ChannelArguments &channelArguments, const Metadata &metadata, QObject *parent)
    : QObject(parent),
      method(method),
      beginTime(std::chrono::steady_clock::now()),
      endTime(beginTime),
      readTag(true),
      writeTag(false) {
    qRegisterMetaType<Metadata>();
    qRegisterMetaType<grpc::ByteBuffer>();
    for (auto iter = metadata.cbegin(); iter != metadata.cend(); iter++) {
        context.AddMetadata(iter.key().toStdString(), iter.value().toStdString());
    }

    channel = grpc::CreateCustomChannel(serverAddress.toStdString(), creds, channelArguments);
    grpc::GenericStub stub(channel);
    call = stub.PrepareCall(&context, method.getRequestPath(), &queue);

    auto watcher = new QueueWatcher(*this);
    watcher->moveToThread(&queueWatcherWorker);
    connect(&queueWatcherWorker, &QThread::finished, watcher, &QObject::deleteLater);
    connect(this, &Session::start, watcher, &QueueWatcher::doWork);
    connect(watcher, &QueueWatcher::messageSent, this, &Session::messageSent);
    connect(watcher, &QueueWatcher::messageReceived, this, &Session::messageReceived);
    connect(watcher, &QueueWatcher::initialMetadataReceived, this, &Session::initialMetadataReceived);
    connect(watcher, &QueueWatcher::trailingMetadataReceived, this, &Session::trailingMetadataReceived);
    connect(watcher, &QueueWatcher::finished, this, &Session::finished);
    connect(watcher, &QueueWatcher::aborted, this, &Session::aborted);
    connect(watcher, &QueueWatcher::finish, this, &Session::finish);
    queueWatcherWorker.start();
    emit start();
}

Session::~Session() {
    queueWatcherWorker.requestInterruption();
    queueWatcherWorker.quit();
    queueWatcherWorker.wait();
}

std::chrono::steady_clock::time_point &Session::getBeginTime() { return beginTime; }

std::chrono::steady_clock::time_point &Session::getEndTime() { return endTime; }

Session::Sequence Session::getSequence() { return sequence; }

void Session::send(const grpc::ByteBuffer &buffer) {
    qDebug() << __FUNCTION__;
    writeBuffer = buffer;
    if (sequence == Sequence::Preparing) {
        call->StartCall(writeTag());
    } else {
        writeTag.advance();
        call->Write(writeBuffer, writeTag());
    }
}

void Session::done() {
    qDebug() << __FUNCTION__;
    if (sequence >= Sequence::WritesDone) {
        qDebug() << "already done!!";
        return;
    }
    sequence = Sequence::WritesDone;
    writeTag.advance();
    call->WritesDone(writeTag());
}

void Session::finish() {
    qDebug() << __FUNCTION__;
    if (sequence >= Sequence::Finishing) {
        qDebug() << "already finished!!";
        return;
    }
    sequence = Sequence::Finishing;
    writeTag.advance();
    call->Finish(&statusBuffer, writeTag());
}

#include "Session.moc"
