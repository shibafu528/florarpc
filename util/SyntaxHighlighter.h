#ifndef FLORARPC_SYNTAXHIGHLIGHTER_H
#define FLORARPC_SYNTAXHIGHLIGHTER_H

#include <KSyntaxHighlighting/syntaxhighlighter.h>

#include <QTextEdit>
#include <memory>

class SyntaxHighlighter {
public:
    static std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> setup(QTextEdit &textEdit, const QPalette &palette);
};

#endif  // FLORARPC_SYNTAXHIGHLIGHTER_H
