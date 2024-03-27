//---------------------------------------------------------------------------
// ported to Qt by Jens Reimann
#include <QShowEvent>
#include <QScreen>
#include <QLabel>
#include <QFileDialog>
#include <QDebug>

#include "refdlg.h"
#include "rtklib.h"

static const QChar degreeChar(0260);           // character code of degree (UTF-8)

//---------------------------------------------------------------------------
RefDialog::RefDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    options = 0;
    position[0] = position[1] = position[2] = roverPosition[0] = roverPosition[1] = roverPosition[2] = 0.0;

    connect(tWStationList, &QTableWidget::cellDoubleClicked, this, &RefDialog::stationListDblClick);
    connect(btnOK, &QPushButton::clicked, this, &RefDialog::btnOKClicked);
    connect(btnCancel, &QPushButton::clicked, this, &RefDialog::reject);
    connect(btnFind, &QPushButton::clicked, this, &RefDialog::findList);
    connect(btnLoad, &QPushButton::clicked, this, &RefDialog::btnLoadClicked);
    connect(lEFind, &QLineEdit::returnPressed, this, &RefDialog::findList);
}
//---------------------------------------------------------------------------
void RefDialog::showEvent(QShowEvent *event)
{
    int width[] = { 30, 80, 90, 65, 50, 80, 55 };

    if (event->spontaneous()) return;

    btnLoad->setVisible(options);

    QStringList columns;
    columns << tr("No") << tr("Latitude(%1)").arg(degreeChar) << tr("Longitude(%1)").arg(degreeChar) << tr("Height(m)") << tr("Id") << tr("Name") << tr("Distance(km)");
    tWStationList->setColumnCount(columns.size());
    tWStationList->setRowCount(2);

    for (int i = 0; i < columns.size(); i++)
        for (int j = 0; j < 2; j++)
            tWStationList->setItem(i, j, new QTableWidgetItem(""));

    fontScale = 2 * physicalDpiX();
    for (int i = 0; i < columns.size(); i++)
        tWStationList->setColumnWidth(i, width[i] * fontScale / 96);

    tWStationList->setHorizontalHeaderLabels(columns);

    loadList();

    tWStationList->sortItems(6);
}
//---------------------------------------------------------------------------
void RefDialog::stationListDblClick(int, int)
{
    if (selectReference())
        accept();
    else
        reject();
}
//---------------------------------------------------------------------------
void RefDialog::btnOKClicked()
{
    if (selectReference())
        accept();
    else
        reject();
}
//---------------------------------------------------------------------------
void RefDialog::btnLoadClicked()
{
    stationPositionFile = QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Load Station List..."), stationPositionFile, tr("Position File (*.pos *.snx);;All (*.*)")));

	loadList();
}
//---------------------------------------------------------------------------
void RefDialog::findList(void)
{
    QString str = lEFind->text();

    QList<QTableWidgetItem *> f = tWStationList->findItems(str, Qt::MatchContains);

    if (f.empty()) return;

    tWStationList->setCurrentItem(f.first());
}
//---------------------------------------------------------------------------
void RefDialog::loadList(void)
{
    QByteArray buff;

    double pos[3];
    int n = 0;

    tWStationList->setRowCount(0);

	// check format
    QFile fp(stationPositionFile);
    if (!fp.open(QIODevice::ReadOnly)) return;

    buff = fp.readAll();
    if (buff.contains("%=SNX")) {
		loadSinex();
		return;
	}

    fp.seek(0);
    while (!fp.atEnd()) {
        buff = fp.readLine();
        buff = buff.mid(0, buff.indexOf('%')); /* remove comments */

        QList<QByteArray> tokens = buff.simplified().split(' ');

        if (tokens.size() != 5) continue;

        tWStationList->setRowCount(++n);

        for (int i = 0; i < 3; i++)
            pos[i] = tokens.at(i).toDouble();

        addReference(n, pos, tokens.at(3), tokens.at(4));
	}
    if (n == 0) tWStationList->setRowCount(0);

    updateDistances();
    setWindowTitle(stationPositionFile);
}
//---------------------------------------------------------------------------
void RefDialog::loadSinex(void)
{
    int n = 0, sol = 0;
    double rr[3], pos[3];
    bool okay;
    QFile file(stationPositionFile);
    QByteArray buff, code;

    if (!file.open(QIODevice::ReadOnly)) return;

    while (!file.atEnd()) {
        buff = file.readLine();

        if (buff.contains("+SOLUTION/ESTIMATE")) sol = 1;
        if (buff.contains("-SOLUTION/ESTIMATE")) sol = 0;

        if (!sol || buff.size() < 68) continue;

        if (buff.mid(7, 4) == "STAX") {
            rr[0] = buff.mid(47, 21).toDouble(&okay);
            if (!okay) continue;
            code = buff.mid(14, 4);
        }
        if (buff.mid(7, 4) == "STAY") {
            rr[1] = buff.mid(47, 21).toDouble(&okay);
            if (!okay) continue;
            if (buff.mid(14, 4) != code) continue;
        }
        if (buff.mid(7, 4) == "STAZ") {
            rr[2] = buff.mid(47, 21).toDouble(&okay);
            if (!okay) continue;
            if (buff.mid(14, 4) != code) continue;
            ecef2pos(rr, pos);
            pos[0] *= R2D;
            pos[1] *= R2D;
            tWStationList->setRowCount(++n);
            addReference(n, pos, code, "");
        }
    }
    ;
    if (n == 0)
        tWStationList->setRowCount(0);

    updateDistances();
    setWindowTitle(stationPositionFile);
}
//---------------------------------------------------------------------------
void RefDialog::addReference(int n, double *pos, const QString code,
               const QString name)
{
    int row = tWStationList->rowCount() - 1;

    tWStationList->setItem(row, 0, new QTableWidgetItem(QString::number(n)));
    tWStationList->setItem(row, 1, new QTableWidgetItem(QString::number(pos[0], 'f', 9)));
    tWStationList->setItem(row, 2, new QTableWidgetItem(QString::number(pos[1], 'f', 9)));
    tWStationList->setItem(row, 3, new QTableWidgetItem(QString::number(pos[2], 'f', 4)));
    tWStationList->setItem(row, 4, new QTableWidgetItem(code));
    tWStationList->setItem(row, 5, new QTableWidgetItem(name));
    tWStationList->setItem(row, 6, new QTableWidgetItem(""));
}
//---------------------------------------------------------------------------
int RefDialog::selectReference(void)
{
    bool ok;

    QList<QTableWidgetItem *> sel = tWStationList->selectedItems();
    if (sel.isEmpty()) return 0;
    int row = tWStationList->row(sel.first());
    position[0] = tWStationList->item(row, 1)->text().toDouble(&ok);
    position[1] = tWStationList->item(row, 2)->text().toDouble(&ok);
    position[2] = tWStationList->item(row, 3)->text().toDouble(&ok);
    stationId = tWStationList->item(row, 4)->text();
    stationName = tWStationList->item(row, 5)->text();
	return 1;
}
//---------------------------------------------------------------------------
void RefDialog::updateDistances(void)
{
    double pos[3], ru[3], rr[3];
    bool ok;

    matcpy(pos, roverPosition, 3, 1);

    if (norm(pos, 3) <= 0.0) return;

    pos[0] *= D2R;
    pos[1] *= D2R;
    pos2ecef(pos, ru);

    for (int i = 1; i < tWStationList->rowCount(); i++) {
        if (tWStationList->item(i, 1)->text() == "") continue;

        pos[0] = tWStationList->item(i, 1)->text().toDouble(&ok) * D2R;
        pos[1] = tWStationList->item(i, 2)->text().toDouble(&ok) * D2R;
        pos[2] = tWStationList->item(i, 3)->text().toDouble(&ok);
        pos2ecef(pos, rr);
        for (int j = 0; j < 3; j++) rr[j] -= ru[j];

        tWStationList->setItem(i, 6, new QTableWidgetItem(QString::number(norm(rr, 3) / 1E3, 'f', 1)));
	}
}
//---------------------------------------------------------------------------
