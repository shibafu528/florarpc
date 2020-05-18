#ifndef FLORARPC_SERVER_H
#define FLORARPC_SERVER_H

#include <QString>
#include <QUuid>

#include "Certificate.h"
#include "florarpc/workspace.pb.h"

class Server {
public:
    Server();
    explicit Server(QUuid id);
    explicit Server(const florarpc::Server &server);

    void writeServer(florarpc::Server &server);

    std::shared_ptr<Certificate> findCertificate(const std::vector<std::shared_ptr<Certificate>> &certificates);

    QUuid id;
    QString name;
    QString address;
    bool useTLS;
    QUuid certificateUUID;
};

#endif  // FLORARPC_SERVER_H
