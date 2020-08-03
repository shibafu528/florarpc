#ifndef FLORARPC_METADATAEDIT_H
#define FLORARPC_METADATAEDIT_H

#include <KSyntaxHighlighting/syntaxhighlighter.h>

#include <QTimer>
#include <QWidget>

#include "entity/Metadata.h"
#include "entity/Session.h"
#include "ui/ui_MetadataEdit.h"

class MetadataEdit : public QWidget {
    Q_OBJECT

public:
    explicit MetadataEdit(QWidget *parent = nullptr);
    bool isValid();
    QString toString();
    void setString(const QString &metadata);
    Session::Metadata toMap();

signals:
    void changed();

private slots:
    void onTextChanged();
    void onValidateTimerTimeout();

private:
    Ui_MetadataEdit ui;
    std::unique_ptr<KSyntaxHighlighting::SyntaxHighlighter> highlighter;
    QTimer *validateTimer;

    bool valid;
    std::unique_ptr<Metadata> metadata;
};

#endif  // FLORARPC_METADATAEDIT_H
