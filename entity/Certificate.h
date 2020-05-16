#ifndef FLORARPC_CERTIFICATE_H
#define FLORARPC_CERTIFICATE_H

#include <QString>
#include <QUuid>

class Certificate {
public:
    Certificate();
    explicit Certificate(QUuid id);

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
