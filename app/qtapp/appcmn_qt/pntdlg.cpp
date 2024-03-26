//---------------------------------------------------------------------------

#include <QShowEvent>
#include <QFileDialog>
#include <QFile>
#include <QDebug>

#include "rtklib.h"
#include "pntdlg.h"

//---------------------------------------------------------------------------
PntDialog::PntDialog(QWidget *parent, Plot *plot)
    : QDialog(parent)
{
    setupUi(this);
    QStringList labels;

    labels << tr("Latitude (°)") << tr("Longitude (°)") << tr("Height (m)") << tr("Name");

    tWPntList->setColumnCount(4);
    tWPntList->setHorizontalHeaderLabels(labels);

    noUpdate = false;

    connect(buttonBox, &QDialogButtonBox::rejected, this, &PntDialog::close);
    connect(btnUpdate, &QPushButton::clicked, this, &PntDialog::updatePoint);
    connect(btnDelete, &QPushButton::clicked, this, &PntDialog::btnDeleteClicked);
    connect(btnAdd, &QPushButton::clicked, this, &PntDialog::btnAddClicked);
    connect(tWPntList, &QTableWidget::itemChanged, this, &PntDialog::updatePoint);
    connect(tWPntList, &QTableWidget::itemSelectionChanged, this, &PntDialog::pntListClicked);
    connect(tWPntList, &QTableWidget::itemDoubleClicked, this, &PntDialog::pntListDblClicked);
}
//---------------------------------------------------------------------------
void PntDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    int width[] = {90, 90, 80, 90};

    int fontScale = QFontMetrics(tWPntList->font()).height() * 4;
    for (int i = 0; i < 4; i++)
        tWPntList->setColumnWidth(i, width[i] * fontScale / 96);

    setPoint();
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
    tWPntList->setItem(tWPntList->rowCount() - 1, 2, new QTableWidgetItem(QString("%1").arg(pos[2], 0, 'f', 4)));
    tWPntList->setItem(tWPntList->rowCount() - 1, 3, new QTableWidgetItem(QString("Point%1").arg(tWPntList->rowCount(), 2, 10, QChar('0'))));
    noUpdate = false;

    updatePoint();
}
//---------------------------------------------------------------------------
void PntDialog::btnDeleteClicked()
{
    QList<QTableWidgetItem *> selections = tWPntList->selectedItems();
    if (selections.isEmpty()) return;

    QTableWidgetItem *sel = selections.first();
    if (!sel) return;

    noUpdate = true;
    tWPntList->removeRow(sel->row());
    noUpdate = false;

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
    plot->selectedWayPoint = -1;
    QList<QTableWidgetItem *> selections = tWPntList->selectedItems();
    if (selections.isEmpty()) return;

    QTableWidgetItem *item = selections.first();
    if (!item) return;

    int sel = tWPntList->row(item);
    plot->selectedWayPoint = sel;

    plot->updatePlot();
}
//---------------------------------------------------------------------------
void PntDialog::pntListDblClicked(QTableWidgetItem *w)
{
    int sel = tWPntList->row(w);

    if ((sel >= plot->nWayPoint) ||(sel < 0)) return;
    plot->setTrackCenter(plot->pointPosition[sel][0], plot->pointPosition[sel][1]);
}
//---------------------------------------------------------------------------
