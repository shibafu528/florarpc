#ifndef FLORARPC_SESSION_H
#define FLORARPC_SESSION_H

#include <QObject>
#include <QMultiMap>
#include <QThread>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/generic/generic_stub_impl.h>
#include "Method.h"

class Session : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(Session)

public:
    typedef QMultiMap<QString, QString> Metadata;

    Session(const Method &method, const QString &serverAddress, const Metadata &metadata, QObject *parent = nullptr);
    ~Session() override;

signals:
    void messageSent();
    void messageReceived(const grpc::ByteBuffer &buffer);
    void initialMetadataReceived(const Session::Metadata &metadata);
    void trailingMetadataReceived(const Session::Metadata &metadata);
    void finished(int code, const QString &message, const QByteArray &details);
    void aborted();
    void start();

public slots:
    void send(const grpc::ByteBuffer &buffer);
    void finish();

private:
    enum class Sequence {
        Preparing,
        Connected,
        Finishing,
    };
    class SequentialTag {
    public:
        explicit SequentialTag(bool reverse) : reverse(reverse) {}
        void* operator()() {
            intptr_t tag = reverse ? -count : count;
            return (void*)tag;
        }
        void advance() {
            if (++count >= INT32_MAX) {
                count = 1;
            }
        }
    private:
        int32_t count = 1;
        bool reverse;
    };
    class QueueWatcher;

    const Method &method;

    grpc::ClientContext context;
    grpc::CompletionQueue queue;
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<grpc::GenericClientAsyncReaderWriter> call;
    QThread queueWatcherWorker;

    Sequence sequence = Sequence::Preparing;
    bool receivedInitialMetadata = false;
    SequentialTag readTag;
    SequentialTag writeTag;
    grpc::ByteBuffer readBuffer;
    grpc::ByteBuffer writeBuffer;
    grpc::Status statusBuffer;

    friend QueueWatcher;
};

Q_DECLARE_METATYPE(Session::Metadata)
Q_DECLARE_METATYPE(grpc::ByteBuffer)

#endif //FLORARPC_SESSION_H
