#include "Server.h"

Server::Server() : Server(QUuid::createUuid()) {}

Server::Server(QUuid id) : id(id), name(), address(), useTLS(false), certificateUUID(), sharedMetadata() {}

Server::Server(const florarpc::Server& server)
    : id(QByteArray::fromStdString(server.id())),
      name(QString::fromStdString(server.name())),
      address(QString::fromStdString(server.address())),
      useTLS(server.usetls()),
      certificateUUID(QByteArray::fromStdString(server.certificate_id())),
      sharedMetadata(QString::fromStdString(server.shared_metadata())),
      tlsTargetNameOverride(QString::fromStdString(server.tls_target_name_override())) {}

void Server::writeServer(florarpc::Server& server) {
    server.set_id(id.toString().toStdString());
    server.set_name(name.toStdString());
    server.set_address(address.toStdString());
    server.set_usetls(useTLS);
    server.set_certificate_id(certificateUUID.toString().toStdString());
    server.set_shared_metadata(sharedMetadata.toStdString());
    server.set_tls_target_name_override(tlsTargetNameOverride.toStdString());
}

std::shared_ptr<Certificate> Server::findCertificate(const std::vector<std::shared_ptr<Certificate>>& certificates) {
    if (!useTLS || certificateUUID.isNull()) {
        return nullptr;
    }

    for (const auto& cert : certificates) {
        if (cert->id == certificateUUID) {
            return cert;
        }
    }
    return nullptr;
}
