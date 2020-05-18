#include "Certificate.h"

Certificate::Certificate() : Certificate(QUuid::createUuid()) {}

Certificate::Certificate(QUuid id) : id(id) {}

Certificate::Certificate(const florarpc::Certificate &certificate)
    : id(QByteArray::fromStdString(certificate.id())),
      name(QString::fromStdString(certificate.name())),
      rootCerts(QByteArray::fromStdString(certificate.root_certs())),
      rootCertsName(QString::fromStdString(certificate.root_certs_name())),
      privateKey(QByteArray::fromStdString(certificate.private_key())),
      privateKeyName(QString::fromStdString(certificate.private_key_name())),
      certChain(QByteArray::fromStdString(certificate.cert_chain())),
      certChainName(QString::fromStdString(certificate.cert_chain_name())) {}

void Certificate::writeCertificate(florarpc::Certificate &certificate) {
    certificate.set_id(id.toString().toStdString());
    certificate.set_name(name.toStdString());
    certificate.set_root_certs(rootCerts);
    certificate.set_root_certs_name(rootCertsName.toStdString());
    certificate.set_private_key(privateKey);
    certificate.set_private_key_name(privateKeyName.toStdString());
    certificate.set_cert_chain(certChain);
    certificate.set_cert_chain_name(certChainName.toStdString());
}
