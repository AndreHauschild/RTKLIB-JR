//---------------------------------------------------------------------------

#include "launchmain.h"
#include "launchoptdlg.h"

#include <QShowEvent>

#include "ui_launchoptdlg.h"

//---------------------------------------------------------------------------
LaunchOptDialog::LaunchOptDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::LaunchOptDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LaunchOptDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &LaunchOptDialog::reject);
}
//---------------------------------------------------------------------------
void LaunchOptDialog::setOption(int option)
{

    if (option == 1) {
        ui->rBOptionMkl->setChecked(true);
	}
    else if (option == 2) {
        ui->rBOptionWin64->setChecked(true);
	}
	else {
        ui->rBOptionNormal->setChecked(true);
	}
}
//---------------------------------------------------------------------------
int LaunchOptDialog::getOption()
{
    if (ui->rBOptionMkl->isChecked()) {
        return 1;
	}
    else if (ui->rBOptionWin64->isChecked()) {
        return 2;
	}
	else {
        return 0;
	}
}
//---------------------------------------------------------------------------
void LaunchOptDialog::setMinimize(int minimize)
{
    ui->cBMinimize->setChecked(minimize);
}
//---------------------------------------------------------------------------
int LaunchOptDialog::getMinimize()
{
    return ui->cBMinimize->isChecked();
}
//---------------------------------------------------------------------------
