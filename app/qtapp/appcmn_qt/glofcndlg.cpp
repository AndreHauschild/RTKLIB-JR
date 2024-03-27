//---------------------------------------------------------------------------

#include <QFileDialog>
#include <QDir>

#include "rtklib.h"
#include "glofcndlg.h"

//---------------------------------------------------------------------------
GloFcnDialog::GloFcnDialog(QWidget* parent) : QDialog(parent)
{
    setupUi(this);
    enableGloFcn = 0;

    for (int i = 0; i < 27; i++) {
        gloFcn[i] = 0;
	}

    connect(btnCancel, &QPushButton::clicked, this, &GloFcnDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &GloFcnDialog::btnOkClicked);
    connect(btnClear, &QPushButton::clicked, this, &GloFcnDialog::btnClearClicked);
    connect(btnRead, &QPushButton::clicked, this, &GloFcnDialog::btnReadClicked);
    connect(cBEnableFcn, &QCheckBox::stateChanged, this, &GloFcnDialog::updateEnable);
}
//---------------------------------------------------------------------------

void GloFcnDialog::showEvent(QShowEvent*)
{
    QString text;
    
    cBEnableFcn->setChecked(enableGloFcn);

    for (int i = 0; i < 27; i++) {
        if (gloFcn[i])
            getFcn(i + 1)->setValue(gloFcn[i] - 8);
        else
            getFcn(i + 1)->setValue(getFcn(i + 1)->minimum());
	}
    updateEnable();
}
//---------------------------------------------------------------------------
void GloFcnDialog::btnOkClicked()
{
	int no;
    
    enableGloFcn = cBEnableFcn->isChecked();
	
    for (int i = 0; i < 27; i++) {
        no = getFcn(i + 1)->value();
        if (no >= -7 && no <= 6) {
            gloFcn[i] = no + 8;
		}
        else gloFcn[i] = 0; // GLONASS FCN+8 (0:none)
	}
    accept();
}
//---------------------------------------------------------------------------
void GloFcnDialog::btnReadClicked()
{
    QString filename, text;
    nav_t nav;
	int prn;

    memset(&nav, 0, sizeof(nav_t));

    filename = QFileDialog::getOpenFileName(this, tr("Open RINEX file"), "", "*.*");
    
    if (!readrnx(qPrintable(filename), 0, "", NULL, &nav, NULL)) return;
	
    for (int i = 0; i < nav.ng; i++) {
        if (satsys(nav.geph[i].sat, &prn) != SYS_GLO) continue;
        getFcn(prn)->setValue(nav.geph[i].frq);
	}
    freenav(&nav, 0xFF);
}
//---------------------------------------------------------------------------
void GloFcnDialog::btnClearClicked()
{
	for (int i=0;i<27;i++) {
        getFcn(i+1)->setValue(getFcn(i + 1)->minimum());
	}
}
//---------------------------------------------------------------------------
void GloFcnDialog::updateEnable(void)
{
    btnClear->setEnabled(cBEnableFcn->isChecked());
    btnRead ->setEnabled(cBEnableFcn->isChecked());
    lblDescription1->setEnabled(cBEnableFcn->isChecked());
    lblDescription1->setEnabled(cBEnableFcn->isChecked());
	
    for (int i = 0; i < 27; i++) {
        getFcn(i + 1)->setEnabled(cBEnableFcn->isChecked());
	}
}
//---------------------------------------------------------------------------
QSpinBox * GloFcnDialog::getFcn(int prn)
{
    QSpinBox *fcn[] = {
        sBFcn01, sBFcn02, sBFcn03, sBFcn04, sBFcn05, sBFcn06, sBFcn07, sBFcn08, sBFcn09, sBFcn10,
        sBFcn11, sBFcn12, sBFcn13, sBFcn14, sBFcn15, sBFcn16, sBFcn17, sBFcn18, sBFcn19, sBFcn20,
        sBFcn21, sBFcn22, sBFcn23, sBFcn24, sBFcn25, sBFcn26, sBFcn27
	};
    return fcn[prn - 1];
}
//---------------------------------------------------------------------------

