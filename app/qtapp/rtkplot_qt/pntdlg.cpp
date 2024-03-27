//---------------------------------------------------------------------------

#include <QShowEvent>
#include <QFileDialog>
#include <QFile>
#include <QDebug>

#include "rtklib.h"
#include "refdlg.h"
#include "pntdlg.h"
#include "plotmain.h"

extern Plot *plot;

//---------------------------------------------------------------------------
PntDialog::PntDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    QStringList labels;

    labels << tr("Latitude (°)") << tr("Longitude (°)") << tr("Height (m)") << tr("Name");

    tWPntList->setColumnCount(3);
    tWPntList->setHorizontalHeaderLabels(labels);

    noUpdate = false;

    connect(btnClose, SIGNAL(clicked(bool)), this, SLOT(close()));
    connect(btnUpdate, SIGNAL(clicked(bool)), this, SLOT(btnUpdateClicked()));
    connect(btnDelete, SIGNAL(clicked(bool)), this, SLOT(btnDeleteClicked()));
    connect(btnAdd, SIGNAL(clicked(bool)), this, SLOT(btnAddClicked()));
    connect(tWPntList, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(pntListSetEditText()));
    connect(tWPntList, SIGNAL(itemSelectionChanged()), this, SLOT(pntListClicked()));
    connect(tWPntList, SIGNAL(itemDoubleClicked(QTableWidgetItem *)), this, SLOT(pntListDblClicked(QTableWidgetItem *)));
}
//---------------------------------------------------------------------------
void PntDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    int width[] = { 90, 90, 80, 90 };

    fontScale = this->physicalDpiX();
    for (int i = 0; i < 4; i++)
        tWPntList->setColumnWidth(i, width[i] * fontScale / 96);
}
//---------------------------------------------------------------------------
void PntDialog::btnAddClicked()
{
    double rr[3], pos[3] = { 0 };

    if (tWPntList->rowCount() >= MAXWAYPNT) return;
    if (!plot->getCenterPosition(rr)) return;
    if (norm(rr, 3) <= 0.0) return;
    ecef2pos(rr, pos);

    noUpdate = true;
    tWPntList->setRowCount(tWPntList->rowCount() + 1);
    tWPntList->setItem(tWPntList->rowCount() - 1, 0, new QTableWidgetItem(QString("%1").arg(pos[0] * R2D, 0, 'f', 9)));
    tWPntList->setItem(tWPntList->rowCount() - 1, 1, new QTableWidgetItem(QString("%1").arg(pos[1] * R2D, 0, 'f', 9)));
    tWPntList->setItem(tWPntList->rowCount() - 1, 2, new QTableWidgetItem(QString("%1").arg(pos[2])));
    tWPntList->setItem(tWPntList->rowCount() - 1, 4, new QTableWidgetItem(QString("Point%1").arg(tWPntList->rowCount(), 2)));
    noUpdate = false;

    updatePoint();
}
//---------------------------------------------------------------------------
void PntDialog::btnDeleteClicked()
{
    QTableWidgetItem *sel = tWPntList->selectedItems().first();;

    if (!sel) return;

    noUpdate = true;
    for (int i = tWPntList->column(sel); i < tWPntList->rowCount(); i++) {
        for (int j = 0; j < tWPntList->columnCount(); j++) {
            if (i + 1 >= tWPntList->rowCount()) tWPntList->setItem(i, j, new QTableWidgetItem(""));
            else tWPntList->setItem(i, j, new QTableWidgetItem(tWPntList->item(i + 1, j)->text()));
		}
	}
    tWPntList->setRowCount(tWPntList->rowCount() - 1);
    noUpdate = false;

    updatePoint();
}
//---------------------------------------------------------------------------
void PntDialog::btnUpdateClicked()
{
    updatePoint();
}
//---------------------------------------------------------------------------
void PntDialog::pntListSetEditText()
{
    updatePoint();
}

//---------------------------------------------------------------------------
void PntDialog::updatePoint()
{
    int n = 0;

    if (noUpdate) return;

    for (int i = 0; i < tWPntList->rowCount(); i++) {
        if (!tWPntList->item(i, 0)) continue;
        if (!tWPntList->item(i, 1)) continue;
        if (!tWPntList->item(i, 2)) continue;
        if (tWPntList->item(i, 2)->text() == "") continue;
        plot->pointPosition[n][0] = tWPntList->item(i, 0)->text().toDouble();
        plot->pointPosition[n][1] = tWPntList->item(i, 1)->text().toDouble();
        plot->pointPosition[n][2] = tWPntList->item(i, 2)->text().toDouble();
        plot->pointName[n++] = tWPntList->item(i, 3)->text();
    }
    plot->nWayPoint = n;

    plot->updatePlot();
}
//---------------------------------------------------------------------------
void PntDialog::setPoint(void)
{
    noUpdate = true;
    tWPntList->setRowCount(plot->nWayPoint);
    for (int i = 0; i < plot->nWayPoint; i++) {
        tWPntList->setItem(i, 0, new QTableWidgetItem(QString::number(plot->pointPosition[i][0], 'f', 9)));
        tWPntList->setItem(i, 1, new QTableWidgetItem(QString::number(plot->pointPosition[i][1], 'f', 9)));
        tWPntList->setItem(i, 2, new QTableWidgetItem(QString::number(plot->pointPosition[i][2], 'f', 4)));
        tWPntList->setItem(i, 3, new QTableWidgetItem(plot->pointName[i]));
    }
    noUpdate = false;
}

//---------------------------------------------------------------------------
void PntDialog::pntListClicked()
{
    QList<QTableWidgetItem *> selections = tWPntList->selectedItems();
    if (selections.isEmpty()) return;
    QTableWidgetItem *item = selections.first();
    if (!item) return;
    int sel = tWPntList->row(item);
    plot->selectedWayPoint = sel < plot->nWayPoint ? sel : -1;
    plot->updatePlot();
}
//---------------------------------------------------------------------------
void PntDialog::pntListDblClicked(QTableWidgetItem *w)
{
    int sel = tWPntList->row(w);

    if (sel >= plot->nWayPoint) return;
    plot->setTrackCenter(plot->pointPosition[sel][0], plot->pointPosition[sel][1]);
}
//---------------------------------------------------------------------------
