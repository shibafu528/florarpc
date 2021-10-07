#include "Certificate.h"

#include <QFileInfo>

static void loadPEMFromFile(std::string *dest, QString &filePath) {
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    *dest = file.readAll().toStdString();
    file.close();
}

Certificate::Certificate() : Certificate(QUuid::createUuid()) {}

Certificate::Certificate(QUuid id) : id(id) {}

Certificate::Certificate(const florarpc::Certificate &certificate)
    : id(QByteArray::fromStdString(certificate.id())),
      name(QString::fromStdString(certificate.name())),
      rootCertsPath(QString::fromStdString(certificate.root_certs_path())),
      rootCertsName(QString::fromStdString(certificate.root_certs_name())),
      privateKeyPath(QString::fromStdString(certificate.private_key_path())),
      privateKeyName(QString::fromStdString(certificate.private_key_name())),
      certChainPath(QString::fromStdString(certificate.cert_chain_path())),
      certChainName(QString::fromStdString(certificate.cert_chain_name())),
      targetNameOverride(QString::fromStdString(certificate.target_name_override())) {}

void Certificate::writeCertificate(florarpc::Certificate &certificate) {
    certificate.set_id(id.toString().toStdString());
    certificate.set_name(name.toStdString());
    certificate.set_root_certs_path(rootCertsPath.toStdString());
    certificate.set_root_certs_name(rootCertsName.toStdString());
    certificate.set_private_key_path(privateKeyPath.toStdString());
    certificate.set_private_key_name(privateKeyName.toStdString());
    certificate.set_cert_chain_path(certChainPath.toStdString());
    certificate.set_cert_chain_name(certChainName.toStdString());
    certificate.set_target_name_override(targetNameOverride.toStdString());

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    // erase dangerous deprecated fields
    certificate.clear_root_certs();
    certificate.clear_private_key();
    certificate.clear_cert_chain();
#ifdef __clang__
#pragma clang diagnostic pop
#elif __GNUC__
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
#endif
}

std::shared_ptr<grpc::ChannelCredentials> Certificate::getCredentials() {
    grpc::SslCredentialsOptions options;
    loadPEMFromFile(&options.pem_root_certs, rootCertsPath);
    loadPEMFromFile(&options.pem_private_key, privateKeyPath);
    loadPEMFromFile(&options.pem_cert_chain, certChainPath);
    return grpc::SslCredentials(options);
}
