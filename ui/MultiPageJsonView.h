#ifndef FLORARPC_MULTIPAGEJSONVIEW_H
#define FLORARPC_MULTIPAGEJSONVIEW_H

#include <QWidget>
#include <KSyntaxHighlighting/syntaxhighlighter.h>
#include <KSyntaxHighlighting/repository.h>
#include "ui/ui_MultiPageJsonView.h"

class MultiPageJsonView : public QWidget {
    Q_OBJECT

public:
    explicit MultiPageJsonView(QWidget *parent = nullptr);
    void setupHighlighter(KSyntaxHighlighting::Repository &repository);

signals:
    void setPagerVisible(bool visible);

public slots:
    void clear();
    void append(const QString &document);

private slots:
    void onPageChanged(int page);
    void onPrevButtonClicked();
    void onNextButtonClicked();

private:
    Ui_MultiPageJsonView ui;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> highlighter;

    QStringList documents;

    void updatePager();
};

#endif //FLORARPC_MULTIPAGEJSONVIEW_H
