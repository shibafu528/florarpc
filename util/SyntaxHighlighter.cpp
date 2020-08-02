#include "SyntaxHighlighter.h"

#include <QApplication>
#include <KSyntaxHighlighting/definition.h>
#include <KSyntaxHighlighting/repository.h>
#include <KSyntaxHighlighting/theme.h>

std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> SyntaxHighlighter::setup(QTextEdit& textEdit, const QPalette &palette) {
    static KSyntaxHighlighting::Repository repository;
    static auto jsonDefinition = repository.definitionForMimeType("application/json");

    if (!jsonDefinition.isValid()) {
        return nullptr;
    }

    const auto theme = (palette.color(QPalette::Base).lightness() < 128)
                       ? repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme)
                       : repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme);
    auto highlighter = std::make_unique<KSyntaxHighlighting::SyntaxHighlighter>(&textEdit);
    auto pal = qApp->palette();
    if (theme.isValid()) {
        pal.setColor(QPalette::Base, theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
        pal.setColor(QPalette::Highlight, theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));
    }
    textEdit.setPalette(pal);

    highlighter->setDefinition(jsonDefinition);
    highlighter->setTheme(theme);
    highlighter->rehighlight();

    return highlighter;
}
