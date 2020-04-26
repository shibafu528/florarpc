#ifndef FLORARPC_SERVER_H
#define FLORARPC_SERVER_H

#include <QString>
#include <QUuid>

class Server {
public:
    Server();
    explicit Server(QUuid id);

    QUuid id;
    QString name;
    QString address;
    bool useTLS;
};

#endif  // FLORARPC_SERVER_H
