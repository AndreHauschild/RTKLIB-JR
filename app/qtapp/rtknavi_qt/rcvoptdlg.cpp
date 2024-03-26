//---------------------------------------------------------------------------
// ported to Qt by Jens Reimann
#include <QShowEvent>
#include "rcvoptdlg.h"

#include <ui_rcvoptdlg.h>


//---------------------------------------------------------------------------
RcvOptDialog::RcvOptDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::RcvOptDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &RcvOptDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &RcvOptDialog::reject);
}
//---------------------------------------------------------------------------
void RcvOptDialog::setOptions(const QString &options)
{
    ui->lEOption->setText(options);
}
//---------------------------------------------------------------------------
QString RcvOptDialog::getOptions()
{
    return ui->lEOption->text();
}
//---------------------------------------------------------------------------
