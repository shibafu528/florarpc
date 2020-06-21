#include "AboutDialog.h"

#include <QFile>
#include <QPushButton>

#include "flora_constants.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint) {
    ui.setupUi(this);

    connect(ui.buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this, &AboutDialog::onOkButtonClick);

    ui.versionLabel->setText(QLatin1String("Version %1").arg(FLORA_VERSION));

    QFile licenses(":/third_party_licenses.md");
    if (licenses.open(QIODevice::ReadOnly)) {
        const auto licensesText = QString::fromUtf8(licenses.readAll());
        ui.thirdPartyLicenses->setMarkdown(licensesText);
        licenses.close();
    }
}

void AboutDialog::onOkButtonClick() { done(Accepted); }
