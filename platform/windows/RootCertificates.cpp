#include "platform/RootCertificates.h"

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <wincrypt.h>

grpc_ssl_roots_override_result Platform::grpc_root_certificates_override_callback(char **pem_root_certs) {
    auto hStore = CertOpenSystemStore(NULL, "ROOT");
    if (hStore == nullptr) {
        return GRPC_SSL_ROOTS_OVERRIDE_FAIL;
    }

    BIO *pemBuffer = BIO_new(BIO_s_mem());
    PCCERT_CONTEXT pCertContext = nullptr;
    while (pCertContext = CertEnumCertificatesInStore(hStore, pCertContext)) {
        const unsigned char *der = pCertContext->pbCertEncoded;
        X509 *x509 = d2i_X509(nullptr, &der, pCertContext->cbCertEncoded);
        if (x509 != nullptr) {
            PEM_write_bio_X509(pemBuffer, x509);
            X509_free(x509);
        }
    }
    CertCloseStore(hStore, 0);

    void *pemData;
    long pemLength = BIO_get_mem_data(pemBuffer, &pemData);
    char *pemDup = static_cast<char *>(malloc(pemLength + 1));
    if (pemDup == nullptr) {
        BIO_free(pemBuffer);
        return GRPC_SSL_ROOTS_OVERRIDE_FAIL;
    }
    pemDup[pemLength] = 0;
    memcpy(pemDup, pemData, pemLength);
    BIO_free(pemBuffer);

    *pem_root_certs = pemDup;

    return GRPC_SSL_ROOTS_OVERRIDE_OK;
}