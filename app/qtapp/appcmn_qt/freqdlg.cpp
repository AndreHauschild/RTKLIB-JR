//---------------------------------------------------------------------------

#include "freqdlg.h"

#include "ui_freqdlg.h"

//---------------------------------------------------------------------------
FreqDialog::FreqDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::FreqDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &FreqDialog::accept);
}
//---------------------------------------------------------------------------
