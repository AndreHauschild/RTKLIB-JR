//---------------------------------------------------------------------------
// ported to Qt by Jens Reimann

#include "aboutdlg.h"
#include "rtklib.h"

//---------------------------------------------------------------------------
AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
}

void AboutDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    QPixmap icon[] = { QPixmap(":/icons/rtk1"),
                       QPixmap(":/icons/rtk2"),
                       QPixmap(":/icons/rtk3"),
                       QPixmap(":/icons/rtk4"),
                       QPixmap(":/icons/rtk5"),
                       QPixmap(":/icons/rtk6"),
                       QPixmap(":/icons/rtk7") };

    if ((iconIndex > 0) && (iconIndex < 7)) wgIcon->setPixmap(icon[iconIndex - 1]);

    lbAbout->setText(aboutString);
    lbVersion->setText(tr("with RTKLIB ver.%1 %2").arg(VER_RTKLIB).arg(PATCH_LEVEL));
    lbCopyright->setText(COPYRIGHT_RTKLIB);

    connect(pbOkay, &QPushButton::clicked, this, &AboutDialog::accept);
}
