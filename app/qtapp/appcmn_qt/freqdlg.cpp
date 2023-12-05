//---------------------------------------------------------------------------

#include "freqdlg.h"
//---------------------------------------------------------------------------
FreqDialog::FreqDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);

    connect(btnOk, &QPushButton::clicked, this, &FreqDialog::accept);

}
//---------------------------------------------------------------------------
