#ifndef FLORARPC_REQUEST_H
#define FLORARPC_REQUEST_H

#include <QObject>

#include "Method.h"
#include "Server.h"

class Request : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString body READ getBody)
    Q_PROPERTY(QHash<QString, QString> metadata READ getMetadata)

public:
    Request(const Method &method, QObject *parent = nullptr);
    const Method &getMethod() const { return method; }
    const std::shared_ptr<Server> &getServer() const { return server; }
    void setServer(const std::shared_ptr<Server> &server) { this->server = server; }
    const QString &getBody() const { return body; }
    void setBody(const QString &body) { this->body = body; }
    const QHash<QString, QString> &getMetadata() const { return metadata; }
    void setMetadata(const QHash<QString, QString> &metadata) { this->metadata = metadata; }

private:
    const Method &method;
    std::shared_ptr<Server> server;
    QString body;
    QHash<QString, QString> metadata;
};

#endif  // FLORARPC_REQUEST_H
