//---------------------------------------------------------------------------

#include <QShowEvent>

#include "rtklib.h"
#include "keydlg.h"
#include "markdlg.h"
#include "refdlg.h"
#include "naviopt.h"
#include "navimain.h"

extern rtksvr_t rtksvr;

//---------------------------------------------------------------------------
QMarkDialog::QMarkDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    nMark = 1;
    fixPosition[0] = fixPosition[1] = fixPosition[2] = 0.0;
    Label1->setText(QString("%%r=%1").arg(nMark, 3, 10, QLatin1Char('0')));

    connect(btnKeyDlg, &QPushButton::clicked, this, &QMarkDialog::btnKeyDlgClicked);
    connect(btnOk, &QPushButton::clicked, this, &QMarkDialog::btnOkClicked);
    connect(btnCancel, &QPushButton::clicked, this, &QMarkDialog::close);
    connect(cBMarkerNameC, &QCheckBox::clicked, this, &QMarkDialog::updateEnable);
    connect(rBGo, &QRadioButton::toggled, this, &QMarkDialog::updateEnable);
    connect(rBStop, &QRadioButton::toggled, this, &QMarkDialog::updateEnable);
    connect(rBFix, &QRadioButton::toggled, this, &QMarkDialog::updateEnable);
    connect(btnPosition, &QPushButton::clicked, this, &QMarkDialog::btnPositionClicked);

    keyDialog = new KeyDialog(this);
}
//---------------------------------------------------------------------------
void QMarkDialog::btnOkClicked()
{
    QString mrkr = cBMarkerName->currentText();
    QString cmnt = lEMarkerComment->text();
    char str2[1024];

    if (rBGo->isChecked()) {
        if (positionMode == PMODE_STATIC || positionMode == PMODE_FIXED)
            positionMode = PMODE_KINEMA;
        else if (positionMode == PMODE_PPP_STATIC || positionMode == PMODE_PPP_FIXED)
            positionMode = PMODE_PPP_KINEMA;
    } else if (rBStop->isChecked()) {
        if (positionMode == PMODE_KINEMA || positionMode == PMODE_FIXED)
            positionMode = PMODE_STATIC;
        else if (positionMode == PMODE_PPP_KINEMA || positionMode == PMODE_PPP_FIXED)
            positionMode = PMODE_PPP_STATIC;
    } else if (rBFix->isChecked()) {
        if (positionMode == PMODE_KINEMA || positionMode == PMODE_STATIC) {
            positionMode = PMODE_FIXED;
        }
        else if (positionMode == PMODE_PPP_KINEMA || positionMode == PMODE_PPP_STATIC) {
            positionMode = PMODE_PPP_FIXED;
        }
    }
    if (cBMarkerNameC->isChecked()) {
        reppath(qPrintable(mrkr), str2, utc2gpst(timeget()), qPrintable(QString("%1").arg(nMark, 3, 10, QChar('0'))), "");
        rtksvrmark(&rtksvr, str2, qPrintable(cmnt));
        nMark++;
        Label1->setText(QString("%%r=%1").arg(nMark, 3, 10, QLatin1Char('0')));
	}
    name = mrkr;
    comment = cmnt;
}
//---------------------------------------------------------------------------
void QMarkDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    if (positionMode == PMODE_STATIC || positionMode == PMODE_PPP_STATIC) {
        rBStop->setChecked(true);
    } else if (positionMode == PMODE_KINEMA || positionMode == PMODE_PPP_KINEMA) {
        rBGo->setChecked(true);
    } else {
        rBStop->setChecked(false);
        rBGo->setChecked(false);
	}
	updateEnable();
}
//---------------------------------------------------------------------------
void QMarkDialog::updateEnable(void)
{
    bool ena = positionMode == PMODE_STATIC || positionMode == PMODE_PPP_STATIC ||
               positionMode == PMODE_KINEMA || positionMode == PMODE_PPP_KINEMA ||
               positionMode == PMODE_FIXED || positionMode == PMODE_PPP_FIXED;

    rBStop->setEnabled(ena);
    rBGo->setEnabled(ena);
    rBFix->setEnabled(ena);
    lblPositionMode->setEnabled(ena);
    sBLatitude->setEnabled(rBFix->isChecked());
    sBLongitude->setEnabled(rBFix->isChecked());
    sBHeight->setEnabled(rBFix->isChecked());
    btnPosition->setEnabled(rBFix->isChecked());
    lblPosition->setEnabled(rBFix->isChecked());
    cBMarkerName->setEnabled(cBMarkerNameC->isChecked());
}
//---------------------------------------------------------------------------
void QMarkDialog::btnKeyDlgClicked()
{
    keyDialog->setWindowTitle(tr("Key Replacement in Marker Name"));
    keyDialog->show();
}
//---------------------------------------------------------------------------
void QMarkDialog::btnPositionClicked()
{
    RefDialog *refDialog =  new RefDialog(this);

    refDialog->position[0] = sBLatitude->value();
    refDialog->position[1] = sBLongitude->value();
    refDialog->position[2] = sBHeight->value();
    refDialog->stationPositionFile = mainForm->optDialog->stationPositionFileF;

    if (refDialog->result() != QDialog::Accepted) return;

    sBLatitude->setValue(refDialog->position[0]);
    sBLongitude->setValue(refDialog->position[1]);
    sBHeight->setValue(refDialog->position[2]);
}
//---------------------------------------------------------------------------
