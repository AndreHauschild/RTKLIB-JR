//---------------------------------------------------------------------------
// ported to Qt by Jens Reimann

#include "aboutdlg.h"
#include "rtklib.h"

#include "ui_aboutdlg.h"


//---------------------------------------------------------------------------
AboutDialog::AboutDialog(QWidget *parent, QPixmap icon, QString aboutString)
    : QDialog(parent), ui(new Ui::AboutDlg)
{
    ui->setupUi(this);

    connect(ui->btnOkay, &QPushButton::clicked, this, &AboutDialog::accept);

    ui->lbIcon->setPixmap(icon.scaled(128, 128));
    ui->lbAbout->setText(aboutString);
    ui->lbVersion->setText(tr("with RTKLIB Version %1 %2").arg(VER_RTKLIB, PATCH_LEVEL));
    ui->lbCopyright->setText(QString("%1\nQt Version\n2016-2025 Jens Reimann").arg(COPYRIGHT_RTKLIB));
}
