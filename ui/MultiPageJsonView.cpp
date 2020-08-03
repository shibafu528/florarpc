#include "MultiPageJsonView.h"

#include "util/SyntaxHighlighter.h"

MultiPageJsonView::MultiPageJsonView(QWidget *parent) : QWidget(parent) {
    ui.setupUi(this);

    connect(this, &MultiPageJsonView::setPagerVisible, ui.pager, &QWidget::setVisible);
    connect(ui.pageSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MultiPageJsonView::onPageChanged);
    connect(ui.prevButton, &QPushButton::clicked, this, &MultiPageJsonView::onPrevButtonClicked);
    connect(ui.nextButton, &QPushButton::clicked, this, &MultiPageJsonView::onNextButtonClicked);

    highlighter = SyntaxHighlighter::setup(*ui.textEdit, palette());

    const auto fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui.textEdit->setFont(fixedFont);

    ui.pager->setDisabled(true);
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
