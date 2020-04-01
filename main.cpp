#include <QApplication>
#include "ui/MainWindow.h"

#ifdef _WIN32
#include <QFont>
#include <QFontDatabase>
#include <QLocale>
#endif

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

#ifdef _WIN32
    // “ú–{ŒêWindows‚ðŽg‚¤l‚ð‹~Ï‚·‚é
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

    auto window = new MainWindow();
    window->show();
    return app.exec();
}
