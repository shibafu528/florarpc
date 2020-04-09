#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include "ui/MainWindow.h"

#ifdef _WIN32
#include <QFont>
#include <QFontDatabase>
#include <QLocale>
#endif

#ifdef __APPLE__
#include <QOperatingSystemVersion>
#include <QFont>
#include <QLocale>
#endif

int main(int argc, char *argv[]) {
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
