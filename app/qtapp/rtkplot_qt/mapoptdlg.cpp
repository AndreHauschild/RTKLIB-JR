//---------------------------------------------------------------------------
#include <stdio.h>
#define INHIBIT_RTK_LOCK_MACROS
#include "rtklib.h"

#include <QShowEvent>
#include <QKeyEvent>
#include <QTextStream>
#include <QFile>
#include <QMessageBox>

#include "plotmain.h"
#include "mapoptdlg.h"

extern Plot *plot;

#define INC_LATLON  0.000001
#define INC_SCALE   0.0001

//---------------------------------------------------------------------------
MapOptDialog::MapOptDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);
    connect(btnSave, SIGNAL(clicked(bool)), this, SLOT(btnSaveClicked()));
    connect(btnClose, SIGNAL(clicked(bool)), this, SLOT(close()));
}
//---------------------------------------------------------------------------
void MapOptDialog::showEvent(QShowEvent*)
{
	updateField();
	updateEnable();
}
//---------------------------------------------------------------------------
void MapOptDialog::btnSaveClicked()
{
    QString file=plot->mapImageFile;
	if (file=="") return;
	file=file+".tag";

    QFile fp(file);
    if (!fp.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (QMessageBox::question(this, file, tr("File exists. Overwrite it?")) != QMessageBox::Yes) return;
	}
    QTextStream out(&fp);
    out << QString("%% map image tag file: rtkplot %1 %2\n\n").arg(VER_RTKLIB).arg(PATCH_LEVEL);
    out << QString("scalex  = %1\n").arg(plot->mapScaleX,0,'g',6);
    out << QString("scaley  = %1\n").arg(plot->mapScaleEqual?plot->mapScaleX:plot->mapScaleY,0,'g',6);
    out << QString("scaleeq = %1\n").arg(plot->mapScaleEqual);
    out << QString("lat     = %1\n").arg(plot->mapLatitude,0,'g',9);
    out << QString("lon     = %1\n").arg(plot->mapLongitude,0,'g',9);

}
//---------------------------------------------------------------------------
void MapOptDialog::btnCenterClicked()
{
    QString s;
	double rr[3],pos[3];
    if (!plot->getCenterPosition(rr)) return;
	ecef2pos(rr,pos);
    sBLatitude->setValue(pos[0]*R2D);
    sBLongitude->setValue(pos[1]*R2D);
	updateMap();
}
//---------------------------------------------------------------------------
void MapOptDialog::btnUpdateClicked()
{
	updateMap();
}
//---------------------------------------------------------------------------
void MapOptDialog::btnCloseClicked()
{
    close();
}
//---------------------------------------------------------------------------
void MapOptDialog::ScaleEqualClicked()
{
	updateMap();
	updateEnable();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void MapOptDialog::updateField(void)
{
    QString s;
    setWindowTitle(plot->mapImageFile);
    sBMapSize1->setValue(plot->mapSize[0]);
    sBMapSize2->setValue(plot->mapSize[1]);
    sBScaleX->setValue(plot->mapScaleX);
    sBScaleY->setValue(plot->mapScaleY);
    sBLatitude->setValue(plot->mapLatitude);
    sBLongitude->setValue(plot->mapLongitude);
    cBScaleEqual->setChecked(plot->mapScaleEqual);
}
//---------------------------------------------------------------------------
void MapOptDialog::updateMap(void)
{
    plot->mapScaleX=sBScaleX->value();
    plot->mapScaleY=sBScaleY->value();
    plot->mapLatitude=sBLatitude->value();
    plot->mapLongitude=sBLongitude->value();
    plot->mapScaleEqual=cBScaleEqual->isChecked();
    plot->updatePlot();
}
//---------------------------------------------------------------------------
void MapOptDialog::updateEnable(void)
{
    sBScaleY      ->setEnabled(!cBScaleEqual->isChecked());
} 
//---------------------------------------------------------------------------

