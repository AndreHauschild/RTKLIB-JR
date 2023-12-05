//---------------------------------------------------------------------------
// ported to Qt by Jens Reimann

#include <QDialog>
#include <QShowEvent>

#include "keydlg.h"

//---------------------------------------------------------------------------
KeyDialog::KeyDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    flag = 0;

    connect(btnOk, &QPushButton::clicked, this, &KeyDialog::close);
}
//---------------------------------------------------------------------------
void KeyDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    Label10->setVisible(flag != 3);
    Label21->setVisible(flag != 3);
    Label23->setVisible(flag != 3);
    Label24->setVisible(flag != 3);
    Label25->setVisible(flag != 3);
    Label26->setVisible(flag != 3);
    Label27->setVisible(flag != 3);
    Label28->setVisible(flag != 3);
    Label29->setVisible(flag >= 1);
    Label30->setVisible(flag >= 1);
    Label31->setVisible(flag == 2);
    Label32->setVisible(flag == 2);
    Label33->setVisible(flag == 3);
    Label34->setVisible(flag == 3);
    Label35->setVisible(flag == 3);
    Label36->setVisible(flag == 3);
    Label37->setVisible(flag == 3);
    Label38->setVisible(flag == 3);
}
//---------------------------------------------------------------------------
