#ifndef FLORARPC_METADATAEDIT_H
#define FLORARPC_METADATAEDIT_H

#include <QTimer>
#include <QWidget>

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

private slots:
    void onTextChanged();
    void onValidateTimerTimeout();

private:
    Ui_MetadataEdit ui;
    QTimer validateTimer;
    bool valid;
    Session::Metadata metadata;
};

#endif  // FLORARPC_METADATAEDIT_H
