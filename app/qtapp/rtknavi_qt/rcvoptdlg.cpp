//---------------------------------------------------------------------------
// ported to Qt5 by Jens Reimann
#include "rcvoptdlg.h"

#include <QShowEvent>

//---------------------------------------------------------------------------
RcvOptDialog::RcvOptDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    connect(btnOk, &QPushButton::clicked, this, &RcvOptDialog::btnOkClicked);
    connect(btnCancel, &QPushButton::clicked, this, &RcvOptDialog::reject);
}
//---------------------------------------------------------------------------
void RcvOptDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    lEOption->setText(options);
}
//---------------------------------------------------------------------------
void RcvOptDialog::btnOkClicked()
{
    options = lEOption->text();

    accept();
}
//---------------------------------------------------------------------------
