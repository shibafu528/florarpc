#ifndef FLORARPC_SERVER_H
#define FLORARPC_SERVER_H

#include <QString>
#include <QUuid>

#include "florarpc/workspace.pb.h"

class Server {
public:
    Server();
    explicit Server(QUuid id);
    explicit Server(const florarpc::Server &server);

    void writeServer(florarpc::Server &server);

    QUuid id;
    QString name;
    QString address;
    bool useTLS;
};

#endif  // FLORARPC_SERVER_H