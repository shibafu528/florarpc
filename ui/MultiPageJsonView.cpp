#include "MultiPageJsonView.h"
#include <KSyntaxHighlighting/definition.h>
#include <KSyntaxHighlighting/theme.h>

MultiPageJsonView::MultiPageJsonView(QWidget *parent) : QWidget(parent) {
    ui.setupUi(this);

    connect(this, &MultiPageJsonView::setPagerVisible, ui.pager, &QWidget::setVisible);
    connect(ui.pageSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MultiPageJsonView::onPageChanged);
    connect(ui.prevButton, &QPushButton::clicked, this, &MultiPageJsonView::onPrevButtonClicked);
    connect(ui.nextButton, &QPushButton::clicked, this, &MultiPageJsonView::onNextButtonClicked);

    const auto fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui.textEdit->setFont(fixedFont);

    ui.pager->setDisabled(true);
}

void MultiPageJsonView::setupHighlighter(KSyntaxHighlighting::Repository &repository) {
    const auto jsonDefinition = repository.definitionForMimeType("application/json");
    if (jsonDefinition.isValid()) {
        const auto theme = (palette().color(QPalette::Base).lightness() < 128) ?
                           repository.defaultTheme(KSyntaxHighlighting::Repository::DarkTheme) :
                           repository.defaultTheme(KSyntaxHighlighting::Repository::LightTheme);

        highlighter = std::make_unique<KSyntaxHighlighting::SyntaxHighlighter>(ui.textEdit);

        auto pal = qApp->palette();
        if (theme.isValid()) {
            pal.setColor(QPalette::Base, theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
            pal.setColor(QPalette::Highlight, theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));
        }
        setPalette(pal);

        highlighter->setDefinition(jsonDefinition);
        highlighter->setTheme(theme);
        highlighter->rehighlight();
    }
}

void MultiPageJsonView::clear() {
    documents.clear();
    ui.pager->setDisabled(true);
}

void MultiPageJsonView::append(const QString &document) {
    documents.append(document);
    updatePager();

    if (documents.size() == 1) {
        onPageChanged(1);
    }
}

void MultiPageJsonView::onPageChanged(int page) {
    if (documents.isEmpty() || page < 1 || page > documents.size()) {
        ui.textEdit->clear();
        return;
    }

    ui.textEdit->setText(documents[page - 1]);
    if (highlighter) {
        highlighter->rehighlight();
    }

    updatePager();
}

void MultiPageJsonView::onPrevButtonClicked() {
    ui.pageSpin->setValue(ui.pageSpin->value() - 1);
    updatePager();
}

void MultiPageJsonView::onNextButtonClicked() {
    ui.pageSpin->setValue(ui.pageSpin->value() + 1);
    updatePager();
}

void MultiPageJsonView::updatePager() {
    ui.pager->setDisabled(false);
    ui.pageSpin->setMaximum(documents.isEmpty() ? 1 : documents.size());
    ui.maxPageLabel->setText(QString("%1").arg(documents.size()));

    if (documents.isEmpty()) {
        ui.prevButton->setDisabled(true);
        ui.nextButton->setDisabled(true);
    } else {
        ui.prevButton->setDisabled(false);
        ui.nextButton->setDisabled(false);
    }

    if (ui.pageSpin->value() <= 1) {
        ui.prevButton->setDisabled(true);
    }
    if (ui.pageSpin->value() == documents.size()) {
        ui.nextButton->setDisabled(true);
    }
}
