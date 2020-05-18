#include "Server.h"

Server::Server() : Server(QUuid::createUuid()) {}

Server::Server(QUuid id) : id(id) {}

Server::Server(const florarpc::Server& server)
    : id(QByteArray::fromStdString(server.id())),
      name(QString::fromStdString(server.name())),
      address(QString::fromStdString(server.address())),
      useTLS(server.usetls()),
      certificateUUID(QByteArray::fromStdString(server.certificate_id())) {}

void Server::writeServer(florarpc::Server& server) {
    server.set_id(id.toString().toStdString());
    server.set_name(name.toStdString());
    server.set_address(address.toStdString());
    server.set_usetls(useTLS);
    server.set_certificate_id(certificateUUID.toString().toStdString());
}
