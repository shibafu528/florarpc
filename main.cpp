#include <grpc/support/log.h>

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTranslator>

#include "entity/Preferences.h"
#include "flora_constants.h"
#include "ui/MainWindow.h"

#ifdef _WIN32
#include <grpc/grpc_security.h>

#include <QFont>
#include <QFontDatabase>
#include <QLocale>

#include "platform/RootCertificates.h"
#endif

#ifdef __APPLE__
#include <QFont>
#include <QLocale>
#include <QOperatingSystemVersion>

#include "platform/RootCertificates.h"
#include "platform/mac/NSWindow.h"
#endif

static MainWindow *mainWindow;
static Preferences *preferences;

void gpr_custom_log_handler(gpr_log_func_args *args) {
    auto message = QString("[%1][gRPC]%2: %3:%4 %5")
                       .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
                       .arg(gpr_log_severity_string(args->severity))
                       .arg(args->file)
                       .arg(args->line)
                       .arg(args->message)
                       .trimmed();
    qDebug() << message.toStdString().c_str();
    QMetaObject::invokeMethod(mainWindow, "onLogging", Qt::QueuedConnection, Q_ARG(QString, message));
}

Preferences &sharedPref() { return *preferences; }

int main(int argc, char *argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    QApplication app(argc, argv);
    QApplication::setApplicationName("FloraRPC");
    QApplication::setApplicationDisplayName("FloraRPC");
    QApplication::setApplicationVersion(FLORA_VERSION);

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
#endif

#ifdef __APPLE__
    // Catalinaでは、漢字が常に中華フォントで描画されてしまう問題がある。
    // -> https://bugreports.qt.io/browse/QTBUG-81924
    // そこで、とりあえずHiragino Sansにフォールバックするようにする。
    // ->
    // https://bugreports.qt.io/browse/QTBUG-81924?focusedCommentId=497035&page=com.atlassian.jira.plugin.system.issuetabpanels%3Acomment-tabpanel#comment-497035
    // TODO: UIテキストをi18nするまでは、ロケールに関係なくフォールバックを書き込んだほうがいいかもしれない。
    QLocale locale;
    if (locale.language() == QLocale::Language::Japanese &&
        QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSCatalina) {
        QFont::insertSubstitution(".AppleSystemUIFont", "Hiragino Sans");
    }

    // 特に対応してもいないタブバーをユーザーが勝手に召喚できてしまうので、封印する
    Platform::Mac::NSWindow::setAllowsAutomaticWindowTabbing(false);
#endif

#if defined(_WIN32) || defined(__APPLE__)
    grpc_set_ssl_roots_override_callback(Platform::grpc_root_certificates_override_callback);
#endif

    gpr_set_log_function(gpr_custom_log_handler);
    gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);

    QTranslator qtTranslator;
    qtTranslator.load(QLocale::system(), "qtbase_", "", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    const auto appConfigDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!appConfigDir.exists() && !appConfigDir.mkpath(".")) {
        QMessageBox::critical(nullptr, "Fatal error", "設定フォルダを作成できませんでした。");
        return EXIT_FAILURE;
    }
    const auto preferenceFilePath = appConfigDir.filePath("preferences.pb");
    preferences = new Preferences(preferenceFilePath);
    preferences->load();

    mainWindow = new MainWindow();
    if (auto args = app.arguments(); args.length() > 1) {
        mainWindow->loadWorkspace(args[1]);
    } else {
        const auto workspace = preferences->read<QString>([](const florarpc::Preferences &pref) {
            if (pref.recent_workspaces_size() < 1) {
                return QString();
            }

            return QString::fromStdString(pref.recent_workspaces(0));
        });

        if (!workspace.isEmpty() && QFile(workspace).exists()) {
            mainWindow->loadWorkspace(workspace);
        }
    }

    mainWindow->show();
    return app.exec();
}
