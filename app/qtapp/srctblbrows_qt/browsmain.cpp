//---------------------------------------------------------------------------
// Ported to Qt by Jens Reimann
#include <clocale>

#include <QCloseEvent>
#include <QShowEvent>
#include <QSettings>
#include <QFileDialog>
#include <QTimer>
#include <QCommandLineParser>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QMetaObject>

#include "rtklib.h"

#include "aboutdlg.h"
#include "mapview.h"
#include "browsmain.h"
#include "staoptdlg.h"
//---------------------------------------------------------------------------

#define PRGNAME                 "NTRIP Browser Qt"
#define PRGVERSION              "1.0"
#define NTRIP_HOME              "rtcm-ntrip.org:2101"   // caster list home
#define NTRIP_TIMEOUT           10000                   // response timeout (ms)
#define MAXSRCTBL               512000                  // max source table size (bytes)
#define ENDSRCTBL               "ENDSOURCETABLE"        // end marker of table
#define ADDRESS_WIDTH           184                     // width of Address (px)

static char buff[MAXSRCTBL];                            // source table buffer

MainForm *mainForm;

extern "C" {
    extern int showmsg(const char *, ...)  {return 0;}
    extern void settime(gtime_t) {}
    extern void settspan(gtime_t, gtime_t) {}
}

/* get source table -------------------------------------------------------*/
static char *getsrctbl(const QString addr)
{
    static int lock = 0;
	stream_t str;
    char *p = buff, msg[MAXSTRMSG];
    unsigned int tick = tickget();

    if (lock) return NULL;
    else lock = 1;

    strinit(&str);

    if (!stropen(&str, STR_NTRIPCLI, STR_MODE_R, qPrintable(addr))) {
        lock = 0;
        QMetaObject::invokeMethod(mainForm, "showMsg", Qt::QueuedConnection, Q_ARG(QString, QT_TR_NOOP("stream open error")));
		return NULL;
	}
    QMetaObject::invokeMethod(mainForm, "showMsg", Qt::QueuedConnection, Q_ARG(QString, QT_TR_NOOP("connecting...")));

    while (p < buff + MAXSRCTBL - 1) {
        int ns = strread(&str, (uint8_t *)p, (buff + MAXSRCTBL - p - 1));
        p += ns; *p='\0';
        qApp->processEvents();
        int stat = strstat(&str, msg);

        QMetaObject::invokeMethod(mainForm, "showMsg", Qt::QueuedConnection, Q_ARG(QString, msg));

        if (stat <= 0) break;
        if (strstr(buff, ENDSRCTBL)) break;
        if ((int)(tickget() - tick) > NTRIP_TIMEOUT) {
            QMetaObject::invokeMethod(mainForm, "showMsg", Qt::QueuedConnection, Q_ARG(QString, QT_TR_NOOP("response timeout")));
			break;
		}
	}
	strclose(&str);
    lock = 0;
	return buff;
}
//---------------------------------------------------------------------------
MainForm::MainForm(QWidget *parent)
    : QMainWindow(parent)
{
    mainForm = this;
    setupUi(this);
    cBFilterFormat->setVisible(false);  // TODO

    QCoreApplication::setApplicationName(PRGNAME);
    QCoreApplication::setApplicationVersion(PRGVERSION);

    setWindowIcon(QIcon(":/icons/srctblbrows_Icon"));

    // retrieve config file name
    QString prg_filename = QApplication::applicationFilePath();
    QFileInfo prg_fileinfo(prg_filename);
    iniFile = prg_fileinfo.absolutePath() + "/" + prg_fileinfo.baseName() + ".ini";

    mapView = new MapView(this);
    staListDialog = new StaListDialog(this);
    loadTimer = new QTimer();

    btnTypeCas->setDefaultAction(actMenuViewCas);
    btnTypeNet->setDefaultAction(actMenuViewNet);
    btnTypeSrc->setDefaultAction(actMenuViewSrc);
    btnTypeStr->setDefaultAction(actMenuViewStr);

    connect(cBAddress, &QComboBox::textActivated, this, &MainForm::addressChanged);
    connect(btnUpdate, &QPushButton::clicked, this, &MainForm::btnUpdateClicked);
    connect(btnMap, &QPushButton::clicked, this, &MainForm::btnMapClicked);
    connect(btnList, &QPushButton::clicked, this, &MainForm::btnListClicked);
    connect(btnSta, &QPushButton::clicked, this, &MainForm::btnStatsionClicked);
    connect(cBStatationMask, &QCheckBox::clicked, this, &MainForm::updateEnable);
    connect(actMenuOpen, &QAction::triggered, this, &MainForm::menuOpenClicked);
    connect(actMenuSave, &QAction::triggered, this, &MainForm::menuSaveClicked);
    connect(actMenuQuit, &QAction::triggered, this, &MainForm::close);
    connect(actMenuUpdateCaster, &QAction::triggered, this, &MainForm::menuUpdateCasterClicked);
    connect(actMenuUpdateTable, &QAction::triggered, this, &MainForm::menuUpdateTableClicked);
    connect(actMenuViewCas, &QAction::triggered, this, &MainForm::menuViewCasterClicked);
    connect(actMenuViewNet, &QAction::triggered, this, &MainForm::menuViewNetClicked);
    connect(actMenuViewSrc, &QAction::triggered, this, &MainForm::menuViewSourceClicked);
    connect(actMenuViewStr, &QAction::triggered, this, &MainForm::menuViewStrClicked);
    connect(actMenuAbout, &QAction::triggered, this, &MainForm::menuAboutClicked);
    connect(loadTimer, &QTimer::timeout, this, &MainForm::loadTimerExpired);
    connect(tWTableStr, &QTableWidget::cellClicked, this, &MainForm::Table0SelectCell);

    btnMap->setEnabled(false);
#ifdef QWEBENGINE
    btnMap->setEnabled(true);
#endif

    tWTableStr->setSortingEnabled(true);
    tWTableCas->setSortingEnabled(true);
    tWTableNet->setSortingEnabled(true);

    statusbar->addWidget(wgMessage);

    strinitcom();

    loadTimer->setInterval(100);
}
//---------------------------------------------------------------------------
void MainForm::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    const QString colw0 = "30,74,116,56,244,18,52,62,28,50,50,18,18,120,28,18,18,40,600,";
    const QString colw1 = "30,112,40,96,126,18,28,50,50,160,40,600,0,0,0,0,0,0,0,";
    const QString colw2 = "30,80,126,18,18,300,300,300,600,0,0,0,0,0,0,0,0,0,0,";
    QSettings setting(iniFile, QSettings::IniFormat);
    QString list, url = "";
    QStringList colw, stas;
    int i;

    QCommandLineParser parser;
    parser.setApplicationDescription(PRGNAME);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("URL", QCoreApplication::translate("main", "file with list of Ntrip sources"));

    parser.process(*QApplication::instance());

    const QStringList args = parser.positionalArguments();

    if (args.count() >= 1) url = args.at(0);

    setWindowTitle(QString("%1 ver.%2 %3").arg(PRGNAME).arg(VER_RTKLIB).arg(PATCH_LEVEL));

    list = setting.value("srctbl/addrlist", "").toString();
    QStringList items = list.split("@");
    foreach(QString item, items){
        cBAddress->addItem(item);
    }

    if (url != "") {
        cBAddress->setCurrentText(url);
        getTable();
    } else {
        cBAddress->setCurrentText(setting.value("srctbl/address", "").toString());
	}

    fontScale = physicalDpiX();

    colw = setting.value("srctbl/colwidth0", colw0).toString().split(",");
    i = 0;
    foreach(QString width, colw){
        tWTableStr->setColumnWidth(i++, width.toDouble() * fontScale / 96);
    }
    colw = setting.value("srctbl/colwidth1", colw1).toString().split(",");
    i = 0;
    foreach(QString width, colw){
        tWTableCas->setColumnWidth(i++, width.toDouble() * fontScale / 96);
    }
    colw = setting.value("srctbl/colwidth2", colw2).toString().split(",");
    i = 0;
    foreach(QString width, colw){
        tWTableNet->setColumnWidth(i++, width.toDouble() * fontScale / 96);
	}

    stationList.clear();
    for (int i = 0; i < 10; i++) {
        stas = setting.value(QString("sta/station%1").arg(i), "").toString().split(",");
        foreach(QString sta, stas){
            stationList.append(sta);
        }
    }

    showTable();
    updateEnable();
    getTable();

    QTimer::singleShot(0, this, &MainForm::updateTable);
}
//---------------------------------------------------------------------------
void MainForm::closeEvent(QCloseEvent *)
{
    QSettings setting(iniFile, QSettings::IniFormat);
    QString list, colw;

    // save table layout
    setting.setValue("srctbl/address", cBAddress->currentText());
    for (int i = 0; i < cBAddress->count(); i++)
        list = list + cBAddress->itemText(i) + "@";
    setting.setValue("srctbl/addrlist", list);

    colw = "";
    for (int i = 0; i < tWTableStr->columnCount(); i++)
        colw = colw + QString::number(tWTableStr->columnWidth(i) * 96 / fontScale);
    setting.setValue("srctbl/colwidth0", colw);

    colw = "";
    for (int i = 0; i < tWTableCas->columnCount(); i++)
        colw = colw + QString::number(tWTableCas->columnWidth(i) * 96 / fontScale);
    setting.setValue("srctbl/colwidth1", colw);

    colw = "";
    for (int i = 0; i < tWTableNet->columnCount(); i++)
        colw = colw + QString::number(tWTableNet->columnWidth(i) * 96 / fontScale);
    setting.setValue("srctbl/colwidth2", colw);

    for (int i = 0, j = 0; i < 10; i++)
    {
        QString buffer;
        for (int k = 0; k < 256 && j < stationList.count(); k++) {
            buffer.append(QStringLiteral("%1%2").arg(k==0?"":",").arg(stationList[j++]));
        };
        setting.setValue(QString("sta/station%1").arg(i), buffer);
    }
}
//---------------------------------------------------------------------------
void MainForm::menuOpenClicked()
{
    QString fileName = QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open"), QString(), tr("All File (*.*)")));
    QFile file(fileName);

    sourceTable = "";

    if (!file.open(QIODevice::ReadOnly)) return;

    sourceTable = file.readAll();

    addressCaster = cBAddress->currentText();

    showTable();
    showMsg(tr("source table loaded"));
}
//---------------------------------------------------------------------------
void MainForm::menuSaveClicked()
{
    QString fileName = QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Save File"), QString(), tr("All File (*.*)")));
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) return;

    file.write(sourceTable.toLatin1());

    showMsg(tr("source table saved"));
}
//---------------------------------------------------------------------------
void MainForm::menuUpdateCasterClicked()
{
    getCaster();
}
//---------------------------------------------------------------------------
void MainForm::menuUpdateTableClicked()
{
    getTable();
}
//---------------------------------------------------------------------------
void MainForm::menuViewStrClicked()
{
    actMenuViewCas->setChecked(false);
    actMenuViewNet->setChecked(false);
    actMenuViewSrc->setChecked(false);

    showTable();
}
//---------------------------------------------------------------------------
void MainForm::menuViewCasterClicked()
{
    actMenuViewStr->setChecked(false);
    actMenuViewNet->setChecked(false);
    actMenuViewSrc->setChecked(false);

    showTable();
}
//---------------------------------------------------------------------------
void MainForm::menuViewNetClicked()
{
    actMenuViewStr->setChecked(false);
    actMenuViewCas->setChecked(false);
    actMenuViewSrc->setChecked(false);

    showTable();
}
//---------------------------------------------------------------------------
void MainForm::menuViewSourceClicked()
{
    actMenuViewStr->setChecked(false);
    actMenuViewCas->setChecked(false);
    actMenuViewNet->setChecked(false);

    showTable();
}
//---------------------------------------------------------------------------
void MainForm::menuAboutClicked()
{
    AboutDialog *aboutDialog = new AboutDialog(this);

    aboutDialog->aboutString = PRGNAME;
    aboutDialog->iconIndex = 7;
    aboutDialog->exec();

    delete aboutDialog;
}
//---------------------------------------------------------------------------
void MainForm::btnMapClicked()
{
    int num = 0;
    float mean_lat = 0, mean_lon = 0, min_lat = 180, max_lat = -180, min_lon = 90, max_lon = -90;
    bool okay;
    loadTimer->start();

    for (int i = 0; i < tWTableStr->rowCount(); i++) {
        if (tWTableStr->item(i, 8)->text() == "") continue;  // country

        QString latitudeText = tWTableStr->item(i, 9)->text();
        QString longitudeText = tWTableStr->item(i, 10)->text();
        float lat = latitudeText.toDouble(&okay); if (!okay) continue;
        float lon = longitudeText.toDouble(&okay); if (!okay) continue;
        mean_lat+=lat;
        mean_lon+=lon;
        if (lat < min_lat) min_lat = lat;
        if (lat > max_lat) max_lat = lat;
        if (lon < min_lon) min_lon = lon;
        if (lon > max_lon) max_lon = lon;
        num++;
    }
    if (num > 0)
    {
        mean_lat/=num;
        mean_lon/=num;

        mapView->setCenter(mean_lat, mean_lon);
    }
    mapView->show();
}
//---------------------------------------------------------------------------
void MainForm::Table0SelectCell(int ARow, int ACol)
{
    Q_UNUSED(ACol);
    QString title;

    if (0 <= ARow && ARow < tWTableStr->rowCount()) {
        title = tWTableStr->item(ARow, 1)->text();
        mapView->highlightMark(ARow);
        mapView->setWindowTitle(QString(tr("NTRIP Data Stream Map: %1/%2")).arg(cBAddress->currentText()).arg(title));
	}
}
//---------------------------------------------------------------------------
void MainForm::btnListClicked()
{
    getCaster();
}
//---------------------------------------------------------------------------
void MainForm::btnUpdateClicked()
{
    getTable();
}
//---------------------------------------------------------------------------
void MainForm::addressChanged()
{
    getTable();
}
//---------------------------------------------------------------------------
void MainForm::loadTimerExpired()
{
    if (!mapView->isLoaded()) return;

    updateMap();

    loadTimer->stop();
}
//---------------------------------------------------------------------------
void MainForm::getCaster(void)
{
    if (casterWatcher.isRunning()) return;

    QString addressText = cBAddress->currentText();
    QString addr = NTRIP_HOME;

    if (addressText != "") addr = addressText;

    btnList->setEnabled(false);
    actMenuUpdateCaster->setEnabled(false);

    // update source table in background and call updateCaster() when finished
    QFuture<char *> tblFuture = QtConcurrent::run(getsrctbl, addr);
    connect(&casterWatcher, &QFutureWatcher<char*>::finished, this, &MainForm::updateCaster);
    casterWatcher.setFuture(tblFuture);
}
//---------------------------------------------------------------------------
void MainForm::updateCaster()
{
    QString currentAddress;
    QString srctbl;

    btnList->setEnabled(true);
    actMenuUpdateCaster->setEnabled(true);

    if ((srctbl = casterWatcher.result()).isEmpty()) return;

    currentAddress = cBAddress->currentText();
    cBAddress->clear();
    cBAddress->setCurrentText(currentAddress);
    cBAddress->addItem("");

    QStringList tokens, lines = srctbl.split('\n');
    foreach(const QString &line, lines) {
        if (!line.contains("CAS")) continue;

        tokens = line.split(";");
        if (tokens.size() < 3) continue;
        cBAddress->addItem(tokens.at(1) + ":" + tokens.at(2), QString(line));
	}
    if (cBAddress->count() > 1) cBAddress->setCurrentIndex(0);
}
//---------------------------------------------------------------------------
void MainForm::getTable()
{
    if (tableWatcher.isRunning()) return;

    QString Address_Text = cBAddress->currentText();
    QString addr = NTRIP_HOME;

    if (Address_Text != "") addr = Address_Text;

    btnUpdate->setEnabled(false);
    cBAddress->setEnabled(false);
    actMenuUpdateTable->setEnabled(false);

    // retrieve data in background and call updateTable() when finished
    QFuture<char *> tblFuture = QtConcurrent::run(getsrctbl, addr);
    connect(&tableWatcher, &QFutureWatcher<char*>::finished, this, &MainForm::updateTable);
    tableWatcher.setFuture(tblFuture);
}
//---------------------------------------------------------------------------
void MainForm::updateTable(void)
{
    QString srctbl;

    btnUpdate->setEnabled(true);
    cBAddress->setEnabled(true);
    actMenuUpdateTable->setEnabled(true);

    srctbl = tableWatcher.result();

    if (!srctbl.isEmpty()) {
        sourceTable = srctbl;
        addressCaster = cBAddress->currentText();
	}

    showTable();
}
//---------------------------------------------------------------------------
void MainForm::showTable(void)
{
    const QString ti[3][19] = {{tr("No"), tr("Mountpoint"), tr("ID"), tr("Format"), tr("Format-Details"), tr("Carrier"), tr("Nav-System"),
                                tr("Network"), tr("Country"), tr("Latitude"), tr("Longitude"), tr("NMEA"), tr("Solution"),
                                tr("Generator"), tr("Compr-Encrp"), tr("Authentication"), tr("Fee"), tr("Bitrate"), "" },
                    { tr("No"), tr("Host"),	tr("Port"),	tr("ID"), tr("Operator"), tr("NMEA"), tr("Country"), tr("Latitude"), tr("Longitude"),
                      tr("Fallback Host"), tr("Fallback Port"), "", "", "", "", "", "", "" },
                    { tr("No"), tr("ID"), tr("Operator"), tr("Authentication"), tr("Fee"), tr("Web-Net"), tr("Web-Str"), tr("Web-Reg"),  "", "","", "",
                      "", "", "", "", "", "" } };


    QTableWidget *table[] = { tWTableStr, tWTableCas, tWTableNet };
    QAction *action[] = { actMenuViewStr, actMenuViewCas, actMenuViewNet, actMenuViewSrc };
    int i, j, numStations, type;

    tETableSrc->setVisible(false);
    for (i = 0; i < 3; i++) table[i]->setVisible(false);

    type = actMenuViewStr->isChecked() ? 0 : (actMenuViewCas->isChecked() ? 1 : (actMenuViewNet->isChecked() ? 2 : 3));
    for (i = 0; i < 4; i++) action[i]->setChecked(i == type);

    if (type == 3) {
        tETableSrc->setVisible(true);
        tETableSrc->setPlainText("");
    } else {
        table[type]->setVisible(true);
        table[type]->setColumnCount(18);
        for (i = 0; i < 18; i++) {
            table[type]->setHorizontalHeaderItem(i, new QTableWidgetItem(ti[type][i]));
		}
	}
    if (addressCaster != cBAddress->currentText()) return;
    if (type == 3) {
        tETableSrc->setPlainText(sourceTable);
		return;
	}
    QStringList lines = sourceTable.split("\n");
    numStations = 0;
    foreach(QString line, lines) {
		switch (type) {
            case 0: if (line.startsWith("STR")) numStations++; break;
            case 1: if (line.startsWith("CAS")) numStations++; break;
            case 2: if (line.startsWith("NET")) numStations++; break;
		}
	}
    if (numStations <= 0) return;

    table[type]->setRowCount(numStations);
    j = 0;
    foreach(QString line, lines) {
		switch (type) {
            case 0: if (line.startsWith("STR")) break; else continue;
            case 1: if (line.startsWith("CAS")) break; else continue;
            case 2: if (line.startsWith("NET")) break; else continue;
		}
        table[type]->setItem(j, 0, new QTableWidgetItem(QString::number(j)));
        QStringList tokens = line.split(";");
        for (int i = 0; i < table[type]->columnCount() && i < tokens.size(); i++)
            table[type]->setItem(j, i, new QTableWidgetItem(tokens.at(i)));
		j++;
	}
    updateMap();
}
//---------------------------------------------------------------------------
void MainForm::showMsg(const QString &msg)
{
    QString str = msg;

    lblMessage->setText(str);
}
//---------------------------------------------------------------------------
void MainForm::updateMap(void)
{
    QString title, msg, latitudeText, longitudeText;
    double latitude, longitude;
    bool okay;

    if (cBAddress->currentText() == "")
        mapView->setWindowTitle(tr("NTRIP Data Stream Map"));
    else
        mapView->setWindowTitle(QString(tr("NTRIP Data Stream Map: %1")).arg(cBAddress->currentText()));
    mapView->clearMark();

    for (int i = 0; i < tWTableStr->rowCount(); i++) {
        if (tWTableStr->item(i, 8)->text() == "") continue;  // country

        latitudeText = tWTableStr->item(i, 9)->text();
        longitudeText = tWTableStr->item(i, 10)->text();
        latitude = latitudeText.toDouble(&okay); if (!okay) continue;
        longitude = longitudeText.toDouble(&okay); if (!okay) continue;

        title = tWTableStr->item(i, 1)->text();
        msg = "<b>" + tWTableStr->item(i, 1)->text() + "</b>: " + tWTableStr->item(i, 2)->text() + " (" + tWTableStr->item(i,8)->text()+")<br>"+
              "Format: "+ tWTableStr->item(i, 3)->text() + ", " + tWTableStr->item(i, 4)->text() + ", <br> " +
              "Nav-Sys: "+tWTableStr->item(i, 6)->text() + "<br>" +
              "Network: "+tWTableStr->item(i, 7)->text() + "<br>" +
              "Latitude/Longitude: "+tWTableStr->item(i, 9)->text()+", "+tWTableStr->item(i, 10)->text()+"<br>"+
              "Generator: "+tWTableStr->item(i, 13)->text();

        mapView->addMark(0, i, latitude, longitude, title, msg);
	}
}
//---------------------------------------------------------------------------
void MainForm::btnStatsionClicked()
{
    staListDialog->exec();
}
//---------------------------------------------------------------------------
void MainForm::updateEnable(void)
{
    btnSta->setEnabled(cBStatationMask->isChecked());
}
//---------------------------------------------------------------------------
