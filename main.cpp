#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include "ui/MainWindow.h"

#ifdef _WIN32
#include <QFont>
#include <QFontDatabase>
#include <QLocale>
#include <grpc/grpc_security.h>
#include <wincrypt.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#endif

#ifdef __APPLE__
#include <QOperatingSystemVersion>
#include <QFont>
#include <QLocale>
#endif

#ifdef _WIN32
grpc_ssl_roots_override_result callbackSSLRootsOverrideWin32(char** pem_root_certs) {
    auto hStore = CertOpenSystemStore(NULL, "ROOT");
    if (hStore == nullptr) {
        return GRPC_SSL_ROOTS_OVERRIDE_FAIL;
    }

    BIO *pemBuffer = BIO_new(BIO_s_mem());
    PCCERT_CONTEXT pCertContext = nullptr;
    while (pCertContext = CertEnumCertificatesInStore(hStore, pCertContext)) {
        const unsigned char* der = pCertContext->pbCertEncoded;
        X509* x509 = d2i_X509(nullptr, &der, pCertContext->cbCertEncoded);
        if (x509 != nullptr) {
            PEM_write_bio_X509(pemBuffer, x509);
            X509_free(x509);
        }
    }
    CertCloseStore(hStore, 0);

    void *pemData;
    long pemLength = BIO_get_mem_data(pemBuffer, &pemData);
    char *pemDup = static_cast<char*>(malloc(pemLength + 1));
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
#endif

int main(int argc, char *argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    QApplication app(argc, argv);

#ifdef _WIN32
    // 日本語Windowsを使う人を救済する
    QLocale locale;
    if (locale.language() == QLocale::Language::Japanese) {
        QFont fontYuGothic("Yu Gothic UI", 9);
        if (fontYuGothic.exactMatch()) {
            app.setFont(fontYuGothic);
        } else {
            QFont fontMeiryo("Meiryo UI", 9);
            app.setFont(fontMeiryo);
        }
    }

    grpc_set_ssl_roots_override_callback(callbackSSLRootsOverrideWin32);
#endif

#ifdef __APPLE__
    // Catalinaでは、漢字が常に中華フォントで描画されてしまう問題がある。
    // -> https://bugreports.qt.io/browse/QTBUG-81924
    // そこで、とりあえずHiragino Sansにフォールバックするようにする。
    // -> https://bugreports.qt.io/browse/QTBUG-81924?focusedCommentId=497035&page=com.atlassian.jira.plugin.system.issuetabpanels%3Acomment-tabpanel#comment-497035
    // TODO: UIテキストをi18nするまでは、ロケールに関係なくフォールバックを書き込んだほうがいいかもしれない。
    QLocale locale;
    if (locale.language() == QLocale::Language::Japanese &&
        QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSCatalina) {
        QFont::insertSubstitution(".AppleSystemUIFont", "Hiragino Sans");
    }
#endif

    QTranslator qtTranslator;
    qtTranslator.load(QLocale::system(), "qtbase_", "", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    auto window = new MainWindow();
    window->show();
    return app.exec();
}
