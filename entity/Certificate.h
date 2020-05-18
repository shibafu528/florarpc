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
    QByteArray rootCerts;
    QString rootCertsName;
    QByteArray privateKey;
    QString privateKeyName;
    QByteArray certChain;
    QString certChainName;
};

#endif  // FLORARPC_CERTIFICATE_H
