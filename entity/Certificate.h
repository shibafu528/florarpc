#ifndef FLORARPC_CERTIFICATE_H
#define FLORARPC_CERTIFICATE_H

#include <grpcpp/security/credentials.h>

#include <QString>
#include <QUuid>

#include "florarpc/workspace.pb.h"

class Certificate {
public:
    Certificate();
    explicit Certificate(QUuid id);
    explicit Certificate(const florarpc::Certificate &certificate);

    void writeCertificate(florarpc::Certificate &certificate);

    std::shared_ptr<grpc::ChannelCredentials> getCredentials();

    QUuid id;
    QString name;
    QString rootCertsPath;
    QString rootCertsName;
    QString privateKeyPath;
    QString privateKeyName;
    QString certChainPath;
    QString certChainName;
};

#endif  // FLORARPC_CERTIFICATE_H
