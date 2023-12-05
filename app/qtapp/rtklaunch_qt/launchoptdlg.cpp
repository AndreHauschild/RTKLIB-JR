//---------------------------------------------------------------------------

#include "launchmain.h"
#include "launchoptdlg.h"

#include <QShowEvent>

extern MainForm *mainForm;

//---------------------------------------------------------------------------
LaunchOptDialog::LaunchOptDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
}
//---------------------------------------------------------------------------
void LaunchOptDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    if (mainForm->option == 1) {
        rBOptionMkl->setChecked(true);
	}
    else if (mainForm->option == 2) {
        rBOptionWin64->setChecked(true);
	}
	else {
        rBOptionNormal->setChecked(true);
	}
    cBMinimize->setChecked(mainForm->minimize);
}
//---------------------------------------------------------------------------
void LaunchOptDialog::btnOkClicked()
{
    if (rBOptionMkl->isChecked()) {
        mainForm->option = 1;
	}
    else if (rBOptionWin64->isChecked()) {
        mainForm->option = 2;
	}
	else {
        mainForm->option = 0;
	}
    mainForm->minimize=cBMinimize->isChecked();
}//---------------------------------------------------------------------------
