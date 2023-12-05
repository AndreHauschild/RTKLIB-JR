//---------------------------------------------------------------------------
// rtknavi : real-time positioning AP
//
//          Copyright (C) 2007-2016 by T.TAKASU, All rights reserved.
//          ported to Qt by Jens Reimann
//
// options : rtknavi [-t title][-i file][-auto][-tray]
//
//           -t title   window title
//           -i file    ini file path
//           -auto      auto start
//           -tray      start as task tray icon
//
// version : $Revision:$ $Date:$
// history : 2008/07/14  1.0 new
//           2010/07/18  1.1 rtklib 2.4.0
//           2010/08/16  1.2 fix bug on setting of satellite antenna model
//           2010/09/04  1.3 fix bug on setting of receiver antenna delta
//           2011/06/10  1.4 rtklib 2.4.1
//           2012/04/03  1.5 rtklib 2.4.2
//           2014/09/06  1.6 rtklib 2.4.3
//           2016/01/31  2.0 ported to Qt
//           2017/09/01  1.7 add option -auto and -tray
//           2020/11/30  1.8 add "Output Velocity" option
//                           support saving multiple sets of ephemeris
//                           fix bug on unable deselecting antenna PCV
//                           fix bug on unable saving TGD2 of ephemeris
//---------------------------------------------------------------------------
#include <stdio.h>
#include <math.h>
#include <clocale>

#include <QMessageBox>
#include <QMainWindow>
#include <QTimer>
#include <QFileInfo>
#include <QFont>
#include <QSettings>
#include <QProcess>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QShowEvent>
#include <QCloseEvent>
#include <QPainter>
#include <QDebug>
#include <QCommandLineParser>

#include "rtklib.h"
#include "instrdlg.h"
#include "outstrdlg.h"
#include "logstrdlg.h"
#include "mondlg.h"
#include "aboutdlg.h"
#include "markdlg.h"
#include "viewer.h"
#include "naviopt.h"
#include "navimain.h"
#include "graph.h"
#include "helper.h"

MainWindow *mainForm;

//---------------------------------------------------------------------------

#define PRGNAME     "RTKNAVI-QT"           // program name
#define TRACEFILE   "rtknavi_%Y%m%d%h%M.trace" // debug trace file
#define STATFILE    "rtknavi_%Y%m%d%h%M.stat"  // solution status file
#define MINSNR      10                  // minimum snr
#define MAXSNR      60                  // maximum snr
#define PANELFONTNAME "Tahoma"
#define PANELFONTSIZE 8
#define POSFONTNAME "Palatino Linotype"
#define POSFONTSIZE 10
#define MIN_BASELINE_LEN    0.01                // minimum baseline length to show

#define KACYCLE     5000                // keep alive cycle (ms)
#define TIMEOUT     10000               // inactive timeout time (ms)
#define DEFAULTPORT 52001               // default monitor port number
#define MAX_PORT_OFFSET  9                   // max port number offset
#define MAXTRKSCALE 23                  // track scale
#define MAXPANELMODE 7                  // max panel mode

#define SQRT(x)     ((x)<0.0||(x)!=(x)?0.0:sqrt(x))
#define MIN(x,y)    ((x)<(y)?(x):(y))

const QChar degreeChar(0260);           // character code of degree (UTF-8)
const QColor ColorSilver(0xc0, 0xc0, 0xc0);
const QColor ColorTeal(0x80, 0x80, 0x00);
const QColor ColorFuchsia(0xff, 0x00, 0xff);
const QColor ColorOrange(0x00, 0xAA, 0xFF);

//---------------------------------------------------------------------------

rtksvr_t rtksvr;                        // rtk server struct
stream_t monistr;                       // monitor stream

// show message in message area ---------------------------------------------
extern "C" {
    extern int showmsg(const char *, ...)
    {
        return 0;
    }
extern void settime(gtime_t) {}
extern void settspan(gtime_t, gtime_t) {}
}
// convert degree to deg-min-sec --------------------------------------------
static void degtodms(double deg, double *dms)
{
    double sgn = 1.0;

    if (deg < 0.0) {
        deg = -deg; sgn = -1.0;
    }
    dms[0] = floor(deg);
    dms[1] = floor((deg - dms[0]) * 60.0);
    dms[2] = (deg - dms[0] - dms[1] / 60.0) * 3600;
    dms[0] *= sgn;
}
// execute command ----------------------------------------------------------
int MainWindow::execCommand(const QString &cmd, const QStringList &opt, int show)
{
    Q_UNUSED(show);

    return QProcess::startDetached(cmd, opt); /* FIXME: show option not yet supported */
}
// constructor --------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QDialog(parent)
{
    mainForm = this;
    setupUi(this);

    setlocale(LC_NUMERIC, "C");

    serverCycle = serverBufferSize = 0;
    solutionBufferSize = 1000;

    for (int i = 0; i < MAXSTRRTK; i++) {
        streamEnabled[i] = streamType[i] = inputFormat[i] = 0;
    }
    for (int i = 0; i < 3; i++)
        commandEnabled[i][0] = commandEnabled[i][1] = commandEnabled[i][2] = 0;

    timeSystem = solutionType = 0;
    for (int i = 0; i < 4; i++) { plotType[i] = frequencyType[i] = baselineMode[i] = trackType[i] = 0; trackScale[i] = 5;};
    solutionsCurrent = solutionsStart = solutionsEnd = numSatellites[0] = numSatellites[1] = 0;
    nMapPoint = 0;
    monitorPortOpen = 0;
    timeStamps = NULL;
    solutionStatus = numValidSatellites = NULL;
    solutionCurrentStatus = 0;
    solutionRover = solutionReference = solutionQr = velocityRover = ages = ratioAR = NULL;

    for (int i = 0; i < 2; i++) for (int j = 0; j < MAXSAT; j++) {
            satellites[i][j] = validSatellites[i][j] = 0;
            satellitesAzimuth[i][j] = satellitesElevation[i][j] = 0.0;
            for (int k = 0; k < NFREQ; k++) satellitesSNR[i][j][k] = 0;
        }
    processingOptions = prcopt_default;
    solutionOptions = solopt_default;

    rtksvrinit(&rtksvr);
    strinit(&monistr);

    setWindowTitle(QString(tr("%1 ver. %2")).arg(PRGNAME).arg(VER_RTKLIB));
    setWindowIcon(QIcon(":/icons/rtknavi_Icon.ico"));

    panelStacking = panelMode = 0;

    for (int i = 0; i < 3; i++)
        trackOrigin[i] = 0.0;
    optDialog = new OptDialog(this);
    inputStrDialog = new InputStrDialog(this);
    outputStrDialog = new OutputStrDialog(this);
    logStrDialog = new LogStrDialog(this);
    monitor = new MonitorDialog(this);
    systemTray = new QSystemTrayIcon(this);

    setTrayIcon(1);

    trayMenu = new QMenu(this);
    timerCycle = timerInactive = autoRun=0;
}

MainWindow::~MainWindow()
{
    delete [] timeStamps;
    delete [] solutionStatus;
    delete [] numValidSatellites;
    delete [] solutionRover;
    delete [] solutionReference;
    delete [] solutionQr;
    delete [] velocityRover;
    delete [] ages;
    delete [] ratioAR;

    rtksvrfree(&rtksvr);
}

// callback on form create --------------------------------------------------
void MainWindow::showEvent(QShowEvent *event)
{
    int taskTray=0;
    if (event->spontaneous()) return;

    trace(3, "showEvent\n");

    updatePlot();

    trayMenu->addAction(tr("Main Window..."), this, &MainWindow::menuExpandClicked);
    trayMenu->addAction(tr("Monitor..."), this, &MainWindow::showMonitorDialog);
    trayMenu->addAction(tr("Plot..."), this, &MainWindow::showRtkPlot);
    trayMenu->addSeparator();
    menuStartAction = trayMenu->addAction(tr("Start"), this, &MainWindow::startClicked);
    menuStopAction = trayMenu->addAction(tr("Stop"), this, &MainWindow::stopClicked);
    trayMenu->addSeparator();
    menuExitAction = trayMenu->addAction(tr("Exit"), this, &MainWindow::exitClicked);

    systemTray->setContextMenu(trayMenu);

    btnStop->setVisible(false);

    connect(systemTray, &QSystemTrayIcon::activated, this, &MainWindow::trayIconClicked);

    connect(btnExit, &QPushButton::clicked, this, &MainWindow::exitClicked);
    connect(btnStart, &QPushButton::clicked, this, &MainWindow::startClicked);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::stopClicked);
    connect(btnAbout, &QPushButton::clicked, this, &MainWindow::btnAboutClicked);
    connect(btnFrequencyType1, &QPushButton::clicked, this, &MainWindow::btnFrequencyType1Clicked);
    connect(btnFrequencyType2, &QPushButton::clicked, this, &MainWindow::btnFrequencyType2Clicked);
    connect(btnFrequencyType3, &QPushButton::clicked, this, &MainWindow::btnFrequencyType3Clicked);
    connect(btnFrequencyType4, &QPushButton::clicked, this, &MainWindow::btnFrequencyType4Clicked);
    connect(btnExpand1, &QPushButton::clicked, this, &MainWindow::btnExpand1Clicked);
    connect(btnShrink1, &QPushButton::clicked, this, &MainWindow::btnShrink1Clicked);
    connect(btnExpand2, &QPushButton::clicked, this, &MainWindow::btnExpand2Clicked);
    connect(btnShrink2, &QPushButton::clicked, this, &MainWindow::btnShrink2Clicked);
    connect(btnExpand3, &QPushButton::clicked, this, &MainWindow::btnExpand3Clicked);
    connect(btnShrink3, &QPushButton::clicked, this, &MainWindow::btnShrink3Clicked);
    connect(btnExpand4, &QPushButton::clicked, this, &MainWindow::btnExpand4Clicked);
    connect(btnShrink4, &QPushButton::clicked, this, &MainWindow::btnShrink4Clicked);
    connect(btnInputStream, &QPushButton::clicked, this, &MainWindow::btnInputStreamClicked);
    connect(btnLogStream, &QPushButton::clicked, this, &MainWindow::btnLogStreamClicked);
    connect(btnMonitor, &QPushButton::clicked, this, &MainWindow::showMonitorDialog);
    connect(btnOptions, &QPushButton::clicked, this, &MainWindow::btnOptionsClicked);
    connect(btnOutputStream, &QPushButton::clicked, this, &MainWindow::btnOutputStreamClicked);
    connect(btnPanel, &QPushButton::clicked, this, &MainWindow::btnPanelClicked);
    connect(btnPlot, &QPushButton::clicked, this, &MainWindow::showRtkPlot);
    connect(btnPlotType1, &QPushButton::clicked, this, &MainWindow::btnPlotType1Clicked);
    connect(btnPlotType2, &QPushButton::clicked, this, &MainWindow::btnPlotType2Clicked);
    connect(btnPlotType3, &QPushButton::clicked, this, &MainWindow::btnPlotType3Clicked);
    connect(btnPlotType4, &QPushButton::clicked, this, &MainWindow::btnPlotType4Clicked);
    connect(btnSave, &QPushButton::clicked, this, &MainWindow::btnSaveClicked);
    connect(btnSolutionType, &QPushButton::clicked, this, &MainWindow::btnSolutionTypeClicked);
    connect(btnSolutionType2, &QPushButton::clicked, this, &MainWindow::btnSolutionTypeClicked);
    connect(btnTaskTray, &QPushButton::clicked, this, &MainWindow::btnTaskTrayClicked);
    connect(btnTimeSys, &QPushButton::clicked, this, &MainWindow::btnTimeSystemClicked);
    connect(btnMark, &QPushButton::clicked, this, &MainWindow::btnMarkClicked);
    connect(sBSolution, &QScrollBar::valueChanged, this, &MainWindow::sBSolutionChanged);
    connect(&updateTimer, &QTimer::timeout, this, &MainWindow::updateTimerTriggered);

    updateTimer.setInterval(100);
    updateTimer.setSingleShot(false);
    updateTimer.start();

#if 1
    QString file = QApplication::applicationFilePath();
    QFileInfo fi(file);
    iniFile = fi.absolutePath() + "/" + fi.baseName() + ".ini";
#else // use unix config path
    QSettings tempSettings(QSettings::IniFormat, QSettings::UserScope, "rtknavi-qt", "rtklib");
    IniFile = tempSettings.fileName();
#endif
    initSolutionBuffer();
    strinitcom();

    QCommandLineParser parser;
    parser.setApplicationDescription("RTK navi");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption iniFileOption(QStringList() << "i" << "ini-file",
                     QCoreApplication::translate("main", "ini file to use"),
                     QCoreApplication::translate("main", "ini file"));
    parser.addOption(iniFileOption);

    QCommandLineOption trayOption(QStringList() << "tray",
                      QCoreApplication::translate("main", "start as task tray icon."));
    parser.addOption(trayOption);

    QCommandLineOption autoOption(QStringList() << "auto",
                      QCoreApplication::translate("main", "auto start."));
    parser.addOption(autoOption);

    parser.process(*QApplication::instance());

    if (parser.isSet(iniFileOption))
        iniFile = parser.value(iniFileOption);

    if (parser.isSet(autoOption)) autoRun = 1;
    if (parser.isSet(trayOption)) taskTray = 1;

    if (taskTray) {
        setVisible(false);
        systemTray->setVisible(true);
    }

    loadOptions();
    loadNavigation(&rtksvr.nav);
    openMonitorPort(monitorPort);

    updatePanel();
    updateTimeSystem();
    updateSolutionType();
    updateFont();
    updatePosition();
    updateEnable();

    if (autoRun) {
        serverStart();
    }
    updateTimer.start();
}
// callback on form close ---------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *)
{
    trace(3, "closeEvent\n");
    updateTimer.stop();
    if (monitorPortOpen > 0) {
        // send disconnect message
        strwrite(&monistr, (uint8_t *)MSG_DISCONN, strlen(MSG_DISCONN));

        strclose(&monistr);
    }
    saveOptions();
    saveNavigation(&rtksvr.nav);
}
// update panel -------------------------------------------------------------
void MainWindow::updatePanel()
{
    // panel modes
    // 0: solution big; 1 panel
    // 1: solution big; 2 panels
    // 2: solution big; 3 panels
    // 3: solution big; 4 panels
    // 4: solution small; 1 panel
    // 5: solution small; 2 panels
    // 6: solution small; 3 panels
    // 7: solution small; 4 panels

    if (panelMode <= 3) {
        panel21->setVisible(true);
        panelSolution ->setVisible(false);
    }
    else {
        panel21->setVisible(false);
        panelSolution ->setVisible(true);
    }

    if (panelMode == 0 || panelMode == 4) {
        panel22->setVisible(true);
        panel23->setVisible(false);
        panel24->setVisible(false);
        panel25->setVisible(false);
    }
    else if (panelMode == 1 || panelMode == 5) {
        panel22->setVisible(true);
        panel23->setVisible(true);
        panel24->setVisible(false);
        panel25->setVisible(false);
    }
    else if (panelMode == 2 || panelMode == 6) {
        panel22->setVisible(true);
        panel23->setVisible(true);
        panel24->setVisible(true);
        panel25->setVisible(false);
    }
    else {
        panel22->setVisible(true);
        panel23->setVisible(true);
        panel24->setVisible(true);
        panel25->setVisible(true);
    }


    if (panelStacking == 0) { // horizontal
        splitter->setOrientation(Qt::Horizontal);
        splitter->setSizes({splitter->size().width()/5, splitter->size().width()/5,
                            splitter->size().width()/5, splitter->size().width()/5, splitter->size().width()/5});
    }
    else { // vertical
        splitter->setOrientation(Qt::Vertical);
        splitter->setSizes({splitter->size().height()/5, splitter->size().height()/5,
                            splitter->size().height()/5, splitter->size().height()/5, splitter->size().height()/5});
    }
}
// update enabled -----------------------------------------------------------
void MainWindow::updateEnable()
{
    btnExpand1->setVisible(plotType[0] == 6);
    btnShrink1->setVisible(plotType[0] == 6);
    btnExpand2->setVisible(plotType[1] == 6);
    btnShrink2->setVisible(plotType[1] == 6);
    btnExpand3->setVisible(plotType[2] == 6);
    btnShrink3->setVisible(plotType[2] == 6);
    btnExpand4->setVisible(plotType[3] == 6);
    btnShrink4->setVisible(plotType[3] == 6);
}
// callback on button-exit --------------------------------------------------
void MainWindow::exitClicked()
{
    trace(3, "btnExitClicked\n");

    close();
}
// callback on button-start -------------------------------------------------
void MainWindow::startClicked()
{
    trace(3, "btnStartClicked\n");

    serverStart();
}
// callback on button-stop --------------------------------------------------
void MainWindow::stopClicked()
{
    trace(3, "btnStopClicked\n");

    serverStop();
}
// callback on button-plot --------------------------------------------------
void MainWindow::showRtkPlot()
{
    QString cmd1 = "rtkplot_qt", cmd2 = "..\\..\\..\\bin\\rtkplot_qt", cmd3 = "..\\rtkplot_qt\\rtkplot_qt";
    QStringList opts;

    trace(3, "btnPlotClicked\n");

    if (monitorPortOpen <= 0) {
        QMessageBox::critical(this, tr("Error"), tr("Monitor port not open"));
        return;
    }

    opts << QString(" -p tcpcli://localhost:%1 -t \"%2 %3\"").arg(monitorPortOpen)
          .arg(windowTitle()).arg(": RTKPLOT QT");

    if (!execCommand(cmd1, opts, 1) && !execCommand(cmd2, opts, 1) && !execCommand(cmd3, opts, 1))
        QMessageBox::critical(this, tr("Error"), tr("error: rtkplot execution"));
}
// callback on button-options -----------------------------------------------
void MainWindow::btnOptionsClicked()
{
    int i, monitorPortChanged = 0;

    trace(3, "btnOptionsClicked\n");

    optDialog->processOptions = processingOptions;
    optDialog->solutionOption = solutionOptions;
    optDialog->debugStatusF = debugStatusLevel;
    optDialog->debugTraceF = debugTraceLevel;
    optDialog->baselineC = baselineEnabled;
    optDialog->baseline[0] = baseline[0];
    optDialog->baseline[1] = baseline[1];

    optDialog->roverPositionTypeF = roverPositionTypeF;
    optDialog->referencePositionTypeF = referencePositionTypeF;
    optDialog->roverAntennaPcvF = roverAntennaPcvF;
    optDialog->referenceAntennaPcvF = referenceAntennaPcvF;
    optDialog->roverAntennaF = roverAntennaF;
    optDialog->referenceAntennaF = referenceAntennaF;

    optDialog->satellitePcvFileF = satellitePcvFileF;
    optDialog->antennaPcvFileF = antennaPcvFileF;
    optDialog->stationPositionFileF = stationPositionFileF;
    optDialog->geoidDataFileF = geoidDataFileF;
    optDialog->dcbFileF = dcbFile;
    optDialog->eopFileF = eopFileF;
    optDialog->localDirectory = localDirectory;

    optDialog->serverCycle = serverCycle;
    optDialog->timeoutTime = timeoutTime;
    optDialog->reconnectTime = reconnectionTime;
    optDialog->nmeaCycle = nmeaCycle;
    optDialog->fileSwapMargin = fileSwapMargin;
    optDialog->serverBufferSize = serverBufferSize;
    optDialog->solutionBufferSize = solutionBufferSize;
    optDialog->savedSolution = savedSolutions;
    optDialog->navSelect = NavSelect;
    optDialog->dgpsCorrection = dgpsCorrection;
    optDialog->sbasCorrection = sbasCorrection;
    optDialog->excludedSatellites = excludedSatellites;
    optDialog->proxyAddr = proxyAddress;
    optDialog->monitorPort = monitorPort;
    optDialog->panelStack = panelStacking;
    panelFont = optDialog->panelFont;
    positionFont = optDialog->positionFont;

    updateFont();
    updatePanel();

    for (i = 0; i < 3; i++) {
        optDialog->roverAntennaDelta[i] = roverAntennaDelta[i];
        optDialog->referenceAntennaDelta[i] = referenceAntennaDelta[i];
        optDialog->roverPosition[i] = roverPosition[i];
        optDialog->referencePosition[i] = referencePosition[i];
    }
    optDialog->exec();

    if (optDialog->result() != QDialog::Accepted) return;

    processingOptions = optDialog->processOptions;
    solutionOptions = optDialog->solutionOption;
    debugStatusLevel = optDialog->debugStatusF;
    debugTraceLevel = optDialog->debugTraceF;
    baselineEnabled = optDialog->baselineC;
    baseline[0] = optDialog->baseline[0];
    baseline[1] = optDialog->baseline[1];

    roverPositionTypeF = optDialog->roverPositionTypeF;
    referencePositionTypeF = optDialog->referencePositionTypeF;
    roverAntennaPcvF = optDialog->roverAntennaPcvF;
    referenceAntennaPcvF = optDialog->referenceAntennaPcvF;
    roverAntennaF = optDialog->roverAntennaF;
    referenceAntennaF = optDialog->referenceAntennaF;

    satellitePcvFileF = optDialog->satellitePcvFileF;
    antennaPcvFileF = optDialog->antennaPcvFileF;
    stationPositionFileF = optDialog->stationPositionFileF;
    geoidDataFileF = optDialog->geoidDataFileF;
    dcbFile = optDialog->dcbFileF;
    eopFileF = optDialog->eopFileF;
    localDirectory = optDialog->localDirectory;

    serverCycle = optDialog->serverCycle;
    timeoutTime = optDialog->timeoutTime;
    reconnectionTime = optDialog->reconnectTime;
    nmeaCycle = optDialog->nmeaCycle;
    fileSwapMargin = optDialog->fileSwapMargin;
    serverBufferSize = optDialog->serverBufferSize;
    savedSolutions = optDialog->savedSolution;
    NavSelect = optDialog->navSelect;
    dgpsCorrection = optDialog->dgpsCorrection;
    sbasCorrection = optDialog->sbasCorrection;
    excludedSatellites = optDialog->excludedSatellites;
    proxyAddress = optDialog->proxyAddr;
    if (monitorPort != optDialog->monitorPort) monitorPortChanged = 1;
    monitorPort = optDialog->monitorPort;
    panelStacking = optDialog->panelStack;

    if (solutionBufferSize != optDialog->solutionBufferSize) {
        solutionBufferSize = optDialog->solutionBufferSize;
        initSolutionBuffer();
        updateTime();
        updatePosition();
        updatePlot();
    }
    for (i = 0; i < 3; i++) {
        roverAntennaDelta[i] = optDialog->roverAntennaDelta[i];
        referenceAntennaDelta[i] = optDialog->referenceAntennaDelta[i];
        roverPosition[i] = optDialog->roverPosition[i];
        referencePosition[i] = optDialog->referencePosition[i];
    }
    positionFont = optDialog->positionFont;
    panelFont = optDialog->panelFont;

    updateFont();
    updatePanel();

    if (!monitorPortChanged) return;

    // send disconnect message
    if (monitorPortOpen > 0) {
        strwrite(&monistr, (uint8_t *)MSG_DISCONN, strlen(MSG_DISCONN));

        strclose(&monistr);
    }
    // reopen monitor stream
    openMonitorPort(monitorPort);
}
// callback on button-input-streams -----------------------------------------
void MainWindow::btnInputStreamClicked()
{
    int i, j;

    trace(3, "btnInputStreamClicked\n");

    for (i = 0; i < 3; i++) {
        inputStrDialog->streamEnabled[i] = streamEnabled[i];
        inputStrDialog->stream[i] = streamType[i];
        inputStrDialog->format[i] = inputFormat[i];
        inputStrDialog->receiverOptions[i] = receiverOptions[i];

        /* Paths[0]:serial,[1]:tcp,[2]:file,[3]:ftp */
        for (j = 0; j < 4; j++) inputStrDialog->paths[i][j] = paths[i][j];
    }
    for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) {
            inputStrDialog->commandEnable[i][j] = commandEnabled[i][j];
            inputStrDialog->commands[i][j] = commands[i][j];
            inputStrDialog->commandEnableTcp[i][j] = commandEnableTcp[i][j];
            inputStrDialog->commandsTcp[i][j] = commandsTcp[i][j];
        }
    for (i = 0; i < 10; i++) {
        inputStrDialog->history[i] = history[i];
    }
    inputStrDialog->nmeaReq = NmeaReq;
    inputStrDialog->timeTag = inputTimeTag;
    inputStrDialog->timeSpeed = inputTimeSpeed;
    inputStrDialog->timeStart = inputTimeStart;
    inputStrDialog->time64Bit = inputTime64Bit;
    inputStrDialog->nmeaPosition[0] = nmeaPosition[0];
    inputStrDialog->nmeaPosition[1] = nmeaPosition[1];
    inputStrDialog->nmeaPosition[2] = nmeaPosition[2];
    inputStrDialog->resetCommand = resetCommand;
    inputStrDialog->maxBaseLine = maxBaseLine;

    inputStrDialog->exec();

    if (inputStrDialog->result() != QDialog::Accepted) return;

    for (i = 0; i < 3; i++) {
        streamEnabled[i] = inputStrDialog->streamEnabled[i];
        streamType[i] = inputStrDialog->stream[i];
        inputFormat [i] = inputStrDialog->format[i];
        receiverOptions [i] = inputStrDialog->receiverOptions[i];
        for (j = 0; j < 4; j++) paths[i][j] = inputStrDialog->paths[i][j];
    }
    for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) {
            commandEnabled[i][j] = inputStrDialog->commandEnable[i][j];
            commands[i][j] = inputStrDialog->commands[i][j];
            commandEnableTcp[i][j] = inputStrDialog->commandEnableTcp[i][j];
            commandsTcp[i][j] = inputStrDialog->commandsTcp[i][j];
        }
    for (i = 0; i < 10; i++) {
        history[i] = inputStrDialog->history[i];
    }
    NmeaReq = inputStrDialog->nmeaReq;
    inputTimeTag = inputStrDialog->timeTag;
    inputTimeSpeed = inputStrDialog->timeSpeed;
    inputTimeStart = inputStrDialog->timeStart;
    inputTime64Bit = inputStrDialog->time64Bit;
    nmeaPosition[0] = inputStrDialog->nmeaPosition[0];
    nmeaPosition[1] = inputStrDialog->nmeaPosition[1];
    nmeaPosition[2] = inputStrDialog->nmeaPosition[2];
    resetCommand = inputStrDialog->resetCommand;
    maxBaseLine = inputStrDialog->maxBaseLine;
}
// confirm overwrite --------------------------------------------------------
int MainWindow::confirmOverwrite(const QString &path)
{
    int itype[] = {STR_SERIAL, STR_TCPCLI, STR_TCPSVR, STR_NTRIPCLI, STR_FILE, STR_FTP, STR_HTTP};
    int i;
    QString buff1, buff2;

    trace(3, "confirmOverwrite\n");

    buff1 = path.mid(path.indexOf("::"));

    if (!QFile::exists(buff1)) return 1; // file not exists

    // check overwrite input files
    for (i = 0; i < 3; i++) {
        if (!streamEnabled[i] || itype[streamType[i]] != STR_FILE) continue;

        buff2 = paths[i][2];
        buff2 = buff2.mid(buff2.indexOf("::"));

        if (buff1 == buff2) {
            lblMessage->setText(QString(tr("Invalid output %1")).arg(buff1));
            return 0;
        }
    }

    return QMessageBox::question(this, tr("File exists"), buff1) == QMessageBox::Yes;
}
// callback on button-output-streams ----------------------------------------
void MainWindow::btnOutputStreamClicked()
{
    int otype[] = {STR_SERIAL, STR_TCPCLI, STR_TCPSVR, STR_NTRIPSVR, STR_NTRIPCAS, STR_FILE};
    int i, j, str, update[2] = { 0 };
    QString path;

    trace(3, "btnOutputStreamClicked\n");

    for (i = 3; i < 5; i++) {
        outputStrDialog->streamEnabled[i - 3] = streamEnabled[i];
        outputStrDialog->stream[i - 3] = streamType[i];
        outputStrDialog->format[i - 3] = inputFormat[i];
        for (j = 0; j < 4; j++) outputStrDialog->paths[i - 3][j] = paths[i][j];
    }
    for (i = 0; i < 10; i++) {
        outputStrDialog->history[i] = history[i];
    }
    outputStrDialog->outputTimeTag = outputTimeTag;
    outputStrDialog->outputAppend = outputAppend;
    outputStrDialog->swapInterval = outputSwapInterval;
    outputStrDialog->exec();

    if (outputStrDialog->result() != QDialog::Accepted) return;

    for (i = 3; i < 5; i++) {
        if (streamEnabled[i] != outputStrDialog->streamEnabled[i - 3] ||
            streamType[i] != outputStrDialog->stream[i - 3] ||
            inputFormat[i] != outputStrDialog->format[i - 3] ||
            paths[i][0] != outputStrDialog->paths[i - 3][0] ||
            paths[i][1] != outputStrDialog->paths[i - 3][1] ||
            paths[i][2] != outputStrDialog->paths[i - 3][2] ||
            paths[i][3] != outputStrDialog->paths[i - 3][3])
                update[i - 3] = 1;
        streamEnabled[i] = outputStrDialog->streamEnabled[i - 3];
        streamType[i] = outputStrDialog->stream[i - 3];
        inputFormat[i] = outputStrDialog->format[i - 3];
        for (j = 0; j < 4; j++) paths[i][j] = outputStrDialog->paths[i - 3][j];
    }
    for (i = 0; i < 10; i++) {
        history[i] = outputStrDialog->history[i];
    }
    outputTimeTag = outputStrDialog->outputTimeTag;
    outputAppend = outputStrDialog->outputAppend;
    outputSwapInterval = outputStrDialog->swapInterval;

    if (btnStart->isEnabled()) return;

    for (i = 3; i < 5; i++) {
        if (!update[i - 3]) continue;

        rtksvrclosestr(&rtksvr, i);

        if (!streamEnabled[i]) continue;

        str = otype[streamType[i]];
        if (str == STR_SERIAL) path = paths[i][0];
        else if (str == STR_FILE) path = paths[i][2];
        else if (str == STR_FTP || str == STR_HTTP) path = paths[i][3];
        else path = paths[i][1];
        if (str == STR_FILE && !confirmOverwrite(path)) {
            streamEnabled[i] = 0;
            continue;
        }
        solutionOptions.posf = inputFormat[i];
        rtksvropenstr(&rtksvr, i, str, qPrintable(path), &solutionOptions);
    }
}
// callback on button-log-streams -------------------------------------------
void MainWindow::btnLogStreamClicked()
{
    int otype[] = {STR_SERIAL, STR_TCPCLI, STR_TCPSVR, STR_NTRIPSVR, STR_NTRIPCAS, STR_FILE};
    int i, j, str, update[3] = {0};
    QString path;

    trace(3, "btnLogStreamClicked\n");

    for (i = 5; i < 8; i++) {
        logStrDialog->streamEnabled[i - 5] = streamEnabled[i];
        logStrDialog->stream [i - 5] = streamType [i];
        for (j = 0; j < 4; j++) logStrDialog->paths[i - 5][j] = paths[i][j];
    }
    for (i = 0; i < 10; i++) {
        logStrDialog->history[i] = history[i];
    }
    logStrDialog->logTimeTag = logTimeTag;
    logStrDialog->logAppend = logAppend;
    logStrDialog->swapInterval = logSwapInterval;

    logStrDialog->exec();

    if (logStrDialog->result() != QDialog::Accepted) return;

    for (i = 5; i < 8; i++) {
        if (streamEnabled[i] != outputStrDialog->streamEnabled[(i - 5) % 2] ||
            streamType[i] != outputStrDialog->stream[(i - 5) % 2] ||
            paths[i][0] != outputStrDialog->paths[(i - 3) % 2][0] ||
            paths[i][1] != outputStrDialog->paths[(i - 3) % 2][1] ||
            paths[i][2] != outputStrDialog->paths[(i - 3) % 2][2] ||
            paths[i][3] != outputStrDialog->paths[(i - 3) % 2][3])
                update[i - 5] = 1;

        streamEnabled[i] = logStrDialog->streamEnabled[i - 5];
        streamType[i] = logStrDialog->stream[i - 5];

        for (j = 0; j < 4; j++) paths[i][j] = logStrDialog->paths[i - 5][j];
    }
    for (i = 0; i < 10; i++) {
        history[i] = logStrDialog->history[i];
    }

    logTimeTag = logStrDialog->logTimeTag;
    logAppend = logStrDialog->logAppend;
    logSwapInterval = logStrDialog->swapInterval;

    if (btnStart->isEnabled()) return;

    for (i = 5; i < 8; i++) {
        if (!update[i - 5]) continue;

        rtksvrclosestr(&rtksvr, i);

        if (!streamEnabled[i]) continue;

        str = otype[streamType[i]];
        if (str == STR_SERIAL) path = paths[i][0];
        else if (str == STR_FILE) path = paths[i][2];
        else if (str == STR_FTP || str == STR_HTTP) path = paths[i][3];
        else path = paths[i][1];
        if (str == STR_FILE && !confirmOverwrite(path)) {
            streamEnabled[i] = 0;
            continue;
        }
        rtksvropenstr(&rtksvr, i, str, qPrintable(path), &solutionOptions);
    }
}
// callback on button-solution-show -----------------------------------------
void MainWindow::btnPanelClicked()
{
    trace(3, "btnPanelClicked\n");

    if (++panelMode > MAXPANELMODE) panelMode = 0;
    updatePanel();
}
// callback on button-plot-type-1 -------------------------------------------
void MainWindow::btnTimeSystemClicked()
{
    trace(3, "btnTimeSystemClicked\n");

    if (++timeSystem > 3) timeSystem = 0;
    updateTimeSystem();
}
// callback on button-solution-type -----------------------------------------
void MainWindow::btnSolutionTypeClicked()
{
    trace(3, "btnSolutionTypeClicked\n");

    if (++solutionType > 4) solutionType = 0;
    updateSolutionType();
}
// callback on button-plottype-1 --------------------------------------------
void MainWindow::btnPlotType1Clicked()
{
    trace(3, "btnPlotType1Clicked\n");

    if (++plotType[0] > 6) plotType[0] = 0;

    updatePlot();
    updatePosition();
    updateEnable();
}
// callback on button-plottype-2 --------------------------------------------
void MainWindow::btnPlotType2Clicked()
{
    trace(3, "btnPlotType2Clicked\n");

    if (++plotType[1] > 6) plotType[1] = 0;

    updatePlot();
    updatePosition();
    updateEnable();
}
//---------------------------------------------------------------------------
void MainWindow::btnPlotType3Clicked()
{
    trace(3,"btnPlotType3Clicked\n");

    if (++plotType[2] > 6) plotType[2] = 0;

    updatePlot();
    updatePosition();
    updateEnable();
}
//---------------------------------------------------------------------------
void MainWindow::btnPlotType4Clicked()
{
    trace(3,"btnPlotType4Clicked\n");

    if (++plotType[3] > 6) plotType[3] = 0;

    updatePlot();
    updatePosition();
    updateEnable();
}
//---------------------------------------------------------------------------
void MainWindow::btnFrequencyTypeChanged(int i)
    {
    if (plotType[i] == 6) {
        if (++trackType[i] > 1) trackType[i] = 0;
        updatePlot();
    } else if (plotType[i] == 5) {
        if (++baselineMode[i] > 1) baselineMode[i] = 0;
        updatePlot();
    } else {
        if (++frequencyType[i] > NFREQ + 1) frequencyType[i] = 0;
        updateSolutionType();
    }
}
// callback on button frequency-type-1 --------------------------------------
void MainWindow::btnFrequencyType1Clicked()
{
    trace(3, "btnFrequencyType1Clicked\n");

    btnFrequencyTypeChanged(0);
}
// callback on button frequency-type-2 --------------------------------------
void MainWindow::btnFrequencyType2Clicked()
{
    trace(3, "btnFrequencyType2Clicked\n");

    btnFrequencyTypeChanged(1);
}
//---------------------------------------------------------------------------
void MainWindow::btnFrequencyType3Clicked()
{
    trace(3,"btnFrequencyType3Clicked\n");

    btnFrequencyTypeChanged(2);
}
//---------------------------------------------------------------------------
void MainWindow::btnFrequencyType4Clicked()
{
    trace(3,"btnFrequencyType4Clicked\n");

    btnFrequencyTypeChanged(3);
}
// callback on button expand-1 ----------------------------------------------
void MainWindow::btnExpand1Clicked()
{
    if (trackScale[0] <= 0) return;
    trackScale[0]--;

    updatePlot();
}
// callback on button shrink-1 ----------------------------------------------
void MainWindow::btnShrink1Clicked()
{
    if (trackScale[0] >= MAXTRKSCALE) return;
    trackScale[0]++;

    updatePlot();
}
// callback on button expand-2 ----------------------------------------------
void MainWindow::btnExpand2Clicked()
{
    if (trackScale[1] <= 0) return;
    trackScale[1]--;

    updatePlot();
}
// callback on button shrink-2 ----------------------------------------------
void MainWindow::btnShrink2Clicked()
{
    if (trackScale[1] >= MAXTRKSCALE) return;
    trackScale[1]++;

    updatePlot();
}
// callback on button expand-3 ----------------------------------------------
void MainWindow::btnExpand3Clicked()
{
    if (trackScale[2] <= 0) return;
    trackScale[2]--;

    updatePlot();
}
// callback on button shrink-3 ----------------------------------------------
void MainWindow::btnShrink3Clicked()
{
    if (trackScale[2] >= MAXTRKSCALE) return;
    trackScale[2]++;

    updatePlot();
}
// callback on button expand-4 ----------------------------------------------
void MainWindow::btnExpand4Clicked()
{
    if (trackScale[3] <= 0) return;
    trackScale[3]--;

    updatePlot();
}
// callback on button shrink-4 ----------------------------------------------
void MainWindow::btnShrink4Clicked()
{
    if (trackScale[3] >= MAXTRKSCALE) return;
    trackScale[3]++;

    updatePlot();
}
// callback on button-rtk-monitor -------------------------------------------
void MainWindow::showMonitorDialog()
{
    trace(3, "btnMonitorDialogClicked\n");

    monitor->setWindowTitle(windowTitle() + ": RTK Monitor");
    monitor->show();
}
// callback on scroll-solution change ---------------------------------------
void MainWindow::sBSolutionChanged()
{
    trace(3, "sBSolutionChanged\n");

    solutionsCurrent = solutionsStart + sBSolution->value();
    if (solutionsCurrent >= solutionBufferSize) solutionsCurrent -= solutionBufferSize;  // wrap around

    updateTime();
    updatePosition();
    updatePlot();
}
// callback on button-save --------------------------------------------------
void MainWindow::btnSaveClicked()
{
    trace(3, "btnSaveClicked\n");

    saveLogs();
}
// callback on button-about -------------------------------------------------
void MainWindow::btnAboutClicked()
{
    QString prog = PRGNAME;

    trace(3, "btnAboutClicked\n");

    aboutDialog = new AboutDialog(this);
    aboutDialog->aboutString = prog;
    aboutDialog->iconIndex = 5;
    aboutDialog->exec();

    delete aboutDialog;
}
// callback on button-tasktray ----------------------------------------------
void MainWindow::btnTaskTrayClicked()
{
    trace(3, "btnTaskTrayClicked\n");

    setVisible(false);
    systemTray->setToolTip(windowTitle());
    systemTray->setVisible(true);
}
// callback on button-tasktray ----------------------------------------------
void MainWindow::trayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    trace(3, "trayIconClicked\n");
    if (reason != QSystemTrayIcon::DoubleClick) return;

    setVisible(true);
    systemTray->setVisible(false);
}
// callback on menu-expand --------------------------------------------------
void MainWindow::menuExpandClicked()
{
    trace(3, "menuExpandClicked\n");

    setVisible(true);
    systemTray->setVisible(false);
}
// start rtk server ---------------------------------------------------------
void MainWindow::serverStart()
{
    solopt_t solopt[2];
    double pos[3], nmeapos[3];
    int itype[] = {
        STR_SERIAL, STR_TCPCLI, STR_TCPSVR, STR_NTRIPCLI, STR_FILE, STR_FTP, STR_HTTP
    };
    int otype[]={
        STR_SERIAL, STR_TCPCLI, STR_TCPSVR, STR_NTRIPSVR, STR_NTRIPCAS, STR_FILE
    };
    int i, strs[MAXSTRRTK] = {0}, sat, ex, stropt[8] = {0};
    char *serverPaths[8], *cmds[3] = {0}, *cmds_periodic[3] = {0}, *rcvopts[3] = {0};
    char buff[1024], *p, errmsg[20148];
    gtime_t time = timeget();
    pcvs_t pcvr, pcvs;
    pcv_t *pcv, pcv0;
    memset(&pcv0, 1, sizeof(pcv_t));

    trace(3, "serverStart\n");

    memset(&pcvr, 0, sizeof(pcvs_t));
    memset(&pcvs, 0, sizeof(pcvs_t));

    lblMessage->setText("");

    if (debugTraceLevel > 0) {
        traceopen(TRACEFILE);
        tracelevel(debugTraceLevel);
    }

    if (roverPositionTypeF <= 2) { // LLH,XYZ
        processingOptions.rovpos = POSOPT_POS;
        processingOptions.ru[0] = roverPosition[0];
        processingOptions.ru[1] = roverPosition[1];
        processingOptions.ru[2] = roverPosition[2];
    } else { // RTCM position
        processingOptions.rovpos = POSOPT_RTCM;
        for (i = 0; i < 3; i++) processingOptions.ru[i] = 0.0;
    }

    if (referencePositionTypeF <= 2) { // LLH,XYZ
        processingOptions.refpos = POSOPT_POS;
        processingOptions.rb[0] = referencePosition[0];
        processingOptions.rb[1] = referencePosition[1];
        processingOptions.rb[2] = referencePosition[2];
    } else if (referencePositionTypeF == 3) { // RTCM/Raw position
        processingOptions.refpos = POSOPT_RTCM;
        for (i = 0; i < 3; i++) processingOptions.rb[i] = 0.0;
    } else { // average of single position
        processingOptions.refpos = POSOPT_SINGLE;
        for (i = 0; i < 3; i++) processingOptions.rb[i] = 0.0;
    }

    for (i = 0; i < MAXSAT; i++)
        processingOptions.exsats[i] = 0;
    if (excludedSatellites != "") { // excluded satellites
        strncpy(buff, qPrintable(excludedSatellites), 1023);
        for (p = strtok(buff, " "); p; p = strtok(NULL, " ")) {
            if (*p == '+') {
                ex = 2; p++;
            } else {
                ex = 1;
            }
            if (!(sat = satid2no(p))) continue;
            processingOptions.exsats[sat - 1] = (unsigned char)ex;
        }
    }
    processingOptions.pcvr[0] = processingOptions.pcvr[1] = pcv0; // initialize antenna PCV
    if ((roverAntennaPcvF || referenceAntennaPcvF) && !readpcv(qPrintable(antennaPcvFileF), &pcvr)) {
        lblMessage->setText(QString(tr("Antenna file read error %1")).arg(antennaPcvFileF));
        return;
    }
    if (roverAntennaPcvF) {
        if ((pcv = searchpcv(0, qPrintable(roverAntennaF), time, &pcvr)))
            processingOptions.pcvr[0] = *pcv;
        else
            lblMessage->setText(QString(tr("No rover antenna PCV: \"%1\"")).arg(qPrintable(roverAntennaF)));
        for (i = 0; i < 3; i++) processingOptions.antdel[0][i] = roverAntennaDelta[i];
    }
    if (referenceAntennaPcvF) {
        if ((pcv = searchpcv(0, qPrintable(referenceAntennaF), time, &pcvr)))
            processingOptions.pcvr[1] = *pcv;
        else
            lblMessage->setText(QString(tr("No reference station antenna PCV: \"%1\"")).arg(qPrintable(referenceAntennaF)));
        for (i = 0; i < 3; i++) processingOptions.antdel[1][i] = referenceAntennaDelta[i];
    }
    if (roverAntennaPcvF || referenceAntennaPcvF)
        free(pcvr.pcv);

    if (processingOptions.sateph == EPHOPT_PREC || processingOptions.sateph == EPHOPT_SSRCOM) {
        if (!readpcv(qPrintable(satellitePcvFileF), &pcvs)) {
            lblMessage->setText(QString(tr("Satellite antenna file read error: %1")).arg(satellitePcvFileF));
            return;
        }
        for (i = 0; i < MAXSAT; i++) {
            if (!(pcv = searchpcv(i + 1, "", time, &pcvs))) continue;
            rtksvr.nav.pcvs[i] = *pcv;
        }
        free(pcvs.pcv);
    }
    if (baselineEnabled) {
        processingOptions.baseline[0] = baseline[0];
        processingOptions.baseline[1] = baseline[1];
    } else {
        processingOptions.baseline[0] = 0.0;
        processingOptions.baseline[1] = 0.0;
    }
    for (i = 0; i < 3; i++) strs[i] = streamEnabled[i] ? itype[streamType[i]] : STR_NONE;
    for (i = 3; i < 5; i++) strs[i] = streamEnabled[i] ? otype[streamType[i]] : STR_NONE;
    for (i = 5; i < 8; i++) strs[i] = streamEnabled[i] ? otype[streamType[i]] : STR_NONE;

    for (i = 0; i < 8; i++) {
        serverPaths[i] = new char[1024];
        serverPaths[i][0] = '\0';
        if (strs[i] == STR_NONE) strncpy(serverPaths[i], "", 1023);
        else if (strs[i] == STR_SERIAL) strncpy(serverPaths[i], qPrintable(paths[i][0]), 1023);
        else if (strs[i] == STR_FILE) strncpy(serverPaths[i], qPrintable(paths[i][2]), 1023);
        else if (strs[i] == STR_FTP || strs[i] == STR_HTTP) strncpy(serverPaths[i], qPrintable(paths[i][3]), 1023);
        else strncpy(serverPaths[i], qPrintable(paths[i][1]), 1023);
    }

    for (i = 0; i < 3; i++) {
        rcvopts[i] = new char[1024];
        cmds[i] = cmds_periodic[i] = NULL;

        if (strs[i] == STR_SERIAL) {
            cmds[i] = new char[1024];
            cmds_periodic[i] = new char[1024];
            if (commandEnabled[i][0]) strncpy(cmds[i], qPrintable(commands[i][0]), 1023);
            if (commandEnabled[i][2]) strncpy(cmds_periodic[i], qPrintable(commands[i][2]), 1023);
        } else if (strs[i] == STR_TCPCLI || strs[i] == STR_TCPSVR ||
               strs[i] == STR_NTRIPCLI) {
            cmds[i] = new char[1024];
            cmds_periodic[i] = new char[1024];
            if (commandEnableTcp[i][0]) strncpy(cmds[i], qPrintable(commandsTcp[i][0]), 1023);
            if (commandEnabled[i][2]) strncpy(cmds_periodic[i], qPrintable(commandsTcp[i][2]), 1023);
        };
        strncpy(rcvopts[i], qPrintable(receiverOptions[i]), 1023);
    }
    nmeaCycle = nmeaCycle < 1000 ? 1000 : nmeaCycle;
    pos[0] = nmeaPosition[0] * D2R;
    pos[1] = nmeaPosition[1] * D2R;
    pos[2] = nmeaPosition[2];
    pos2ecef(pos, nmeapos);

    strsetdir(qPrintable(localDirectory));
    strsetproxy(qPrintable(proxyAddress));

    for (i = 3; i < 8; i++)
        if (strs[i] == STR_FILE && !confirmOverwrite(serverPaths[i])) return;

    if (debugStatusLevel > 0)
        rtkopenstat(STATFILE, debugStatusLevel);
    if (solutionOptions.geoid > 0 && geoidDataFileF != "")
        opengeoid(solutionOptions.geoid, qPrintable(geoidDataFileF));
    if (dcbFile != "")
        readdcb(qPrintable(dcbFile), &rtksvr.nav, NULL);
    for (i = 0; i < 2; i++) {
        solopt[i] = solutionOptions;
        solopt[i].posf = inputFormat[i + 3];
    }
    stropt[0] = timeoutTime;
    stropt[1] = reconnectionTime;
    stropt[2] = 1000;
    stropt[3] = serverBufferSize;
    stropt[4] = fileSwapMargin;
    strsetopt(stropt);
    strncpy(rtksvr.cmd_reset, qPrintable(resetCommand), 4095);
    rtksvr.bl_reset = maxBaseLine;

    // start rtk server
    if (!rtksvrstart(&rtksvr, serverCycle, serverBufferSize, strs, serverPaths, inputFormat, NavSelect,
                     cmds, cmds_periodic, rcvopts, nmeaCycle, NmeaReq, nmeapos,
                     &processingOptions, solopt, &monistr, errmsg)) {

        trace(2,"rtksvrstart error %s\n",errmsg);
        traceclose();
        for (i = 0; i < 8; i++) delete[] serverPaths[i];
        for (i = 0; i < 3; i++) delete[] rcvopts[i];
        for (i = 0; i < 3; i++)
            if (cmds[i]) delete[] cmds[i];
        for (i = 0; i < 3; i++)
            if (cmds_periodic[i]) delete[] cmds_periodic[i];
        return;
    }

    for (i = 0; i < 8; i++) delete[] serverPaths[i];
    for (i = 0; i < 3; i++) delete[] rcvopts[i];
    for (i = 0; i < 3; i++)
        if (cmds[i]) delete[] cmds[i];
    for (i = 0; i < 3; i++)
        if (cmds_periodic[i]) delete[] cmds_periodic[i];

    solutionsCurrent = solutionsStart = solutionsEnd = 0;
    solutionStatus[0] = numValidSatellites[0] = 0;

    for (i = 0; i < 3; i++) solutionRover[i] = solutionReference[i] = velocityRover[i] = 0.0;
    for (i = 0; i < 9; i++) solutionQr[i] = 0.0;

    ages[0] = ratioAR[0] = 0.0;
    numSatellites[0] = numSatellites[1] = 0;

    updatePosition();
    updatePlot();

    btnStart->setVisible(false);
    btnOptions->setEnabled(false);
    btnExit->setEnabled(false);
    btnInputStream->setEnabled(false);
    menuStartAction->setEnabled(false);
    menuExitAction->setEnabled(false);
    sBSolution->setEnabled(false);
    btnStop->setVisible(true);
    menuStopAction->setEnabled(true);
    lblServer->setStyleSheet("QLabel {background-color: rgb(255,128,0);}");

    setTrayIcon(0);
}
// strop rtk server ---------------------------------------------------------
void MainWindow::serverStop()
{
    char *cmds[3] = { 0 };
    int i, n, m, str;

    trace(3, "serverStop\n");

    for (i = 0; i < 3; i++) {
        cmds[i] = NULL;
        str = rtksvr.stream[i].type;

        if (str == STR_SERIAL) {
            cmds[i] = new char[1024];
            if (commandEnabled[i][1]) strncpy(cmds[i], qPrintable(commands[i][1]), 1023);
        } else if (str == STR_TCPCLI || str == STR_TCPSVR || str == STR_NTRIPCLI) {
            cmds[i] = new char[1024];
            if (commandEnableTcp[i][1]) strncpy(cmds[i], qPrintable(commandsTcp[i][1]), 1023);
        }
    }
    rtksvrstop(&rtksvr, cmds);

    for (i = 0; i < 3; i++) delete[] cmds[i];

    btnStart->setVisible(true);
    btnOptions->setEnabled(true);
    btnExit->setEnabled(true);
    btnInputStream->setEnabled(true);
    menuStartAction->setEnabled(true);
    menuExitAction->setEnabled(true);
    sBSolution->setEnabled(true);
    btnStop->setVisible(false);
    menuStopAction->setEnabled(false);
    lblServer->setStyleSheet("QLabel {background-color: gray;}");

    setTrayIcon(1);

    lblTime->setStyleSheet("QLabel {color: gray;}");
    lblIndicatorSolution->setStyleSheet("QLabel {color: white; background-color: white;}");

    n = solutionsEnd - solutionsStart; if (n < 0) n += solutionBufferSize;
    m = solutionsCurrent - solutionsStart;  if (m < 0) m += solutionBufferSize;
    if (n > 0) {
        sBSolution->setMaximum(n - 1);
        sBSolution->setValue(m);
    }
    lblMessage->setText("");

    if (debugTraceLevel > 0) traceclose();
    if (debugStatusLevel > 0) rtkclosestat();
    if (solutionOptions.geoid > 0 && geoidDataFileF != "") closegeoid();
}
// callback on interval timer -----------------------------------------------
void MainWindow::updateTimerTriggered()
{
    sol_t *sol;
    int i, update = 0;

    trace(4, "updateTimerTriggered\n");

    timerCycle++;

    rtksvrlock(&rtksvr);

    for (i = 0; i < rtksvr.nsol; i++) {
        sol = rtksvr.solbuf + i;
        updateLog(sol->stat, sol->time, sol->rr, sol->qr, rtksvr.rtk.rb, sol->ns,
                  sol->age, sol->ratio);
        update = 1;
    }
    rtksvr.nsol = 0;
    solutionCurrentStatus = rtksvr.state ? rtksvr.rtk.sol.stat : 0;

    rtksvrunlock(&rtksvr);

    if (update) {
        updateTime();
        updatePosition();
        timerInactive = 0;
    } else {
        if (++timerInactive * updateTimer.interval() > TIMEOUT) solutionCurrentStatus = 0;
    }

    if (solutionCurrentStatus) {
        lblServer->setStyleSheet("QLabel {background-color: rgb(0,255,0);}");
        lblTime->setStyleSheet("QLabel { color: black;}");
    } else {
        lblIndicatorSolution->setStyleSheet("QLabel {color: white; background-color: white;}");
        lblSolution->setStyleSheet("QLabel {color: gray;}");
        lblServer->setStyleSheet(rtksvr.state ? "QLabel {background-color: green; }" : "QLabel {background-color: gray; }");
    }

    if (!(++timerCycle % 5)) updatePlot();

    updateStream();

    // keep alive for monitor port
    if (!(++timerCycle % (KACYCLE / updateTimer.interval())) && monitorPortOpen) {
        unsigned char buf[1];
        buf[0] = '\r';
        strwrite(&monistr, buf, 1);
    }
}

// update time-system -------------------------------------------------------
void MainWindow::updateTimeSystem()
{
    QString label[] = {tr("GPST"), tr("UTC"), tr("LT"), tr("GPST")};

    trace(3, "updateTimeSystem\n");

    btnTimeSys->setText(label[timeSystem]);

    updateTime();
}
// update solution type -----------------------------------------------------
void MainWindow::updateSolutionType()
{
    QString label[] = {
        tr("Lat/Lon/Height"), tr("Lat/Lon/Height"), tr("X/Y/Z-ECEF"), tr("E/N/U-Baseline"),
        tr("Pitch/Yaw/Length-Baseline"), ""
    };

    trace(3, "updateSolutionType\n");

    lblSolutionText->setText(label[solutionType]);

    updatePosition();
}
// update log ---------------------------------------------------------------
void MainWindow::updateLog(int stat, gtime_t time, double *rr,
               float *qr, double *rb, int ns, double age, double ratio)
{
    int i;

    if (!stat) return;

    trace(4, "updateLog\n");

    solutionStatus[solutionsEnd] = stat;
    timeStamps[solutionsEnd] = time;
    numValidSatellites[solutionsEnd] = ns;
    ages[solutionsEnd] = age;
    ratioAR[solutionsEnd] = ratio;
    for (i = 0; i < 3; i++) {
        solutionRover[i + solutionsEnd * 3] = rr[i];
        solutionReference[i + solutionsEnd * 3] = rb[i];
        velocityRover[i + solutionsEnd * 3] = rr[i + 3];
    }
    solutionQr[solutionsEnd * 9] = qr[0];
    solutionQr[solutionsEnd * 9 + 4] = qr[1];
    solutionQr[solutionsEnd * 9 + 8] = qr[2];
    solutionQr[solutionsEnd * 9 + 1] = solutionQr[solutionsEnd * 9 + 3] = qr[3];
    solutionQr[solutionsEnd * 9 + 5] = solutionQr[solutionsEnd * 9 + 7] = qr[4];
    solutionQr[solutionsEnd * 9 + 2] = solutionQr[solutionsEnd * 9 + 6] = qr[5];

    solutionsCurrent = solutionsEnd;
    // check for wrap around of ring buffer
    if (++solutionsEnd >= solutionBufferSize) solutionsEnd = 0;
    if (solutionsEnd == solutionsStart && ++solutionsStart >= solutionBufferSize) solutionsStart = 0;
}
// update font --------------------------------------------------------------
void MainWindow::updateFont()
{
    QLabel *label[] = {
        lblSolutionText, lblPositionText1, lblPositionText2, lblPositionText3, lblPosition1, lblPosition2, lblPosition3, lblSolution, lblStd, lblNSatellites
    };
    QString color = label[7]->styleSheet();

    trace(4, "UpdateFont\n");

    for (int i = 0; i < 10; i++) {
        label[i]->setFont(positionFont);
        label[8]->setStyleSheet(QString("QLabel {color: %1;}").arg(color2String(positionFontColor)));
    }
    QFont tmp = positionFont;
    tmp.setPointSize(9);
    label[0]->setFont(tmp);
    label[7]->setStyleSheet(color);
    tmp.setPointSize(8);
    label[8]->setFont(tmp); label[8]->setStyleSheet("QLabel {color: gray;}");
    label[9]->setFont(tmp); label[9]->setStyleSheet("QLabel {color: gray;}");
}
// update time --------------------------------------------------------------
void MainWindow::updateTime()
{
    gtime_t time = timeStamps[solutionsCurrent];
    struct tm *t;
    double tow;
    int week;
    char tstr[64];
    QString str;

    trace(4, "updateTime\n");

    if (timeSystem == 0) {  // GPST
        time2str(time, tstr, 1);
    } else if (timeSystem == 1) {  // UTC
        time2str(gpst2utc(time), tstr, 1);
    } else if (timeSystem == 2) {  // local time
        time = gpst2utc(time);
        if (!(t = localtime(&time.time))) str = "2000/01/01 00:00:00.0";
        else str = QString("%1/%2/%3 %4:%5:%6.%7").arg(t->tm_year + 1900, 4, 10, QChar('0'))
               .arg(t->tm_mon + 1, 2, 10, QChar('0')).arg(t->tm_mday, 2, 10, QChar('0')).arg(t->tm_hour, 2, 10, QChar('0')).arg(t->tm_min, 2, 10, QChar('0'))
               .arg(t->tm_sec, 2, 10, QChar('0')).arg(static_cast<int>(time.sec * 10));
    } else if (timeSystem == 3) {  // GPS time (week & TOW)
        tow = time2gpst(time, &week);
        str = QString("week %1 %2 s").arg(week, 4, 10, QChar('0')).arg(tow, 8, 'f', 1);
    }
    lblTime->setText(str);
}
// update solution display --------------------------------------------------
void MainWindow::updatePosition()
{
    QLabel *label[] = {lblPositionText1, lblPositionText2, lblPositionText3, lblPosition1, lblPosition2, lblPosition3, lblStd, lblNSatellites};
    QString sol[] = {tr("----"), tr("FIX"), tr("FLOAT"), tr("SBAS"), tr("DGPS"), tr("SINGLE"), tr("PPP")};
    QString s[9], ext = "";
    QString color[] = {"silver", "green", "rgb(0,170,255)", "rgb(255,0,255)", "blue", "red", "rgb(128,0,128)"};
    double *rrover = solutionRover + solutionsCurrent * 3;
    double *rbase = solutionReference + solutionsCurrent * 3;
    double *qrover = solutionQr + solutionsCurrent * 9;
    double pos[3] = {0}, Qe[9] = {0};
    double dms1[3] = {0}, dms2[3] = {0}, baseline[3] = {0}, enu[3] = {0}, pitch = 0.0, yaw = 0.0, len;
    int i, stat = solutionStatus[solutionsCurrent];

    trace(4, "updatePosition\n");

    if (rtksvr.rtk.opt.mode == PMODE_STATIC || rtksvr.rtk.opt.mode == PMODE_PPP_STATIC)
        ext = " (S)";
    else if (rtksvr.rtk.opt.mode == PMODE_FIXED || rtksvr.rtk.opt.mode == PMODE_PPP_FIXED)
        ext = " (F)";
    lblSolutionText->setText(tr("Solution%1:").arg(ext));

    lblSolution->setText(sol[stat]);
    lblSolution->setStyleSheet(QString("QLabel {color: %1;}").arg(rtksvr.state ? color[stat] : "gray"));
    lblIndicatorSolution->setStyleSheet(QString("QLabel {color: %1; background-color: %1}").arg(rtksvr.state && stat ? color[stat] : "white"));
    lblIndicatorSolution->setToolTip(sol[stat]);

    if (norm(rrover, 3) > 0.0 && norm(rbase, 3) > 0.0)
        for (i = 0; i < 3; i++)
            baseline[i] = rrover[i] - rbase[i];

    len = norm(baseline, 3);
    if (solutionType == 0) {  // DMS
        if (norm(rrover, 3) > 0.0) {
            ecef2pos(rrover, pos);
            covenu(pos, qrover, Qe);
            degtodms(pos[0] * R2D, dms1);
            degtodms(pos[1] * R2D, dms2);
            if (solutionOptions.height == 1) pos[2] -= geoidh(pos); /* geodetic */
        }
        s[0] = pos[0] < 0 ? tr("S:") : tr("N:");
        s[1] = pos[1] < 0 ? tr("W:") : tr("E:");
        s[2] = solutionOptions.height == 1 ? "H:" : "He:";
        s[3] = QString("%1%2 %3' %4\"").arg(fabs(dms1[0]), 0, 'f', 0).arg(degreeChar).arg(dms1[1], 2, 'f', 0, '0').arg(dms1[2], 7, 'f', 4, '0');
        s[4] = QString("%1%2 %3' %4\"").arg(fabs(dms2[0]), 0, 'f', 0).arg(degreeChar).arg(dms2[1], 2, 'f', 0, '0').arg(dms2[2], 7, 'f', 4, '0');
        s[5] = QString("%1 m").arg(pos[2], 0, 'f', 3);
        s[6] = QString(tr("N:%1 E:%2 U:%3 m")).arg(SQRT(Qe[4]), 6, 'f', 3).arg(SQRT(Qe[0]), 6, 'f', 3).arg(SQRT(Qe[8]), 6, 'f', 3);
    } else if (solutionType == 1) {  // deg
        if (norm(rrover, 3) > 0.0) {
            ecef2pos(rrover, pos); covenu(pos, qrover, Qe);
            if (solutionOptions.height == 1) pos[2] -= geoidh(pos); /* geodetic */
        }
        s[0] = pos[0] < 0 ? "S:" : "N:";
        s[1] = pos[1] < 0 ? "W:" : "E:";
        s[2] = solutionOptions.height == 1 ? "H:" : "He:";
        s[3] = QString("%1 %2").arg(fabs(pos[0]) * R2D, 0, 'f', 8).arg(degreeChar);
        s[4] = QString("%1 %2").arg(fabs(pos[1]) * R2D, 0, 'f', 8).arg(degreeChar);
        s[5] = QString("%1").arg(pos[2], 0, 'f', 3);
        s[6] = QString(tr("N:%1 E:%2 U:%3 m")).arg(SQRT(Qe[4]), 6, 'f', 3).arg(SQRT(Qe[0]), 6, 'f', 3).arg(SQRT(Qe[8]), 6, 'f', 3);
    } else if (solutionType == 2) {  // XYZ
        s[0] = "X:";
        s[1] = "Y:";
        s[2] = "Z:";
        s[3] = QString("%1 m").arg(rrover[0], 0, 'f', 3);
        s[4] = QString("%1 m").arg(rrover[1], 0, 'f', 3);
        s[5] = QString("%1 m").arg(rrover[2], 0, 'f', 3);
        s[6] = QString("X:%1 Y:%2 Z:%3 m").arg(SQRT(qrover[0]), 6, 'f', 3).arg(SQRT(qrover[4]), 6, 'f', 3).arg(SQRT(qrover[8]), 6, 'f', 3);
    } else if (solutionType == 3) {  // ENU
        if (len > 0.0) {
            ecef2pos(rbase, pos);
            ecef2enu(pos, baseline, enu);
            covenu(pos, qrover, Qe);
        }
        s[0] = "E:"; s[1] = "N:"; s[2] = "U:";
        s[3] = QString("%1 m").arg(enu[0], 0, 'f', 3);
        s[4] = QString("%1 m").arg(enu[1], 0, 'f', 3);
        s[5] = QString("%1 m").arg(enu[2], 0, 'f', 3);
        s[6] = QString(tr("N:%1 E:%2 U:%3 m")).arg(SQRT(Qe[4]), 6, 'f', 3).arg(SQRT(Qe[0]), 6, 'f', 3).arg(SQRT(Qe[8]), 6, 'f', 3);
    } else {  // pitch/yaw/len
        if (len > 0.0) {
            ecef2pos(rbase, pos);
            ecef2enu(pos, baseline, enu);
            covenu(pos, qrover, Qe);
            pitch = asin(enu[2] / len);
            yaw = atan2(enu[0], enu[1]); if (yaw < 0.0) yaw += 2.0 * PI;
        }
        s[0] = "P:";
        s[1] = "Y:";
        s[2] = "L:";
        s[3] = QString("%1 %2").arg(pitch * R2D, 0, 'f', 3).arg(degreeChar);
        s[4] = QString("%1 %2").arg(yaw * R2D, 0, 'f', 3).arg(degreeChar);
        s[5] = QString("%1 m").arg(len, 0, 'f', 3);
        s[6] = QString(tr("N:%1 E:%2 U:%3 m")).arg(SQRT(Qe[4]), 6, 'f', 3).arg(SQRT(Qe[0]), 6, 'f', 3).arg(SQRT(Qe[8]), 6, 'f', 3);
    }
    s[7] = QString(tr("Age:%1 s Ratio:%2 #Sat:%3")).arg(ages[solutionsCurrent], 4, 'f', 1).arg(ratioAR[solutionsCurrent], 4, 'f', 1).arg(numValidSatellites[solutionsCurrent], 2);

    if (ratioAR[solutionsCurrent] > 0.0)
        s[8] = QString(" R:%1").arg(ratioAR[solutionsCurrent], 4, 'f', 1);

    for (i = 0; i < 8; i++) label[i]->setText(s[i]);
    for (i = 3; i < 6; i++)
        label[i]->setStyleSheet(QString("QLabel {color: %1;}").arg(processingOptions.mode == PMODE_MOVEB && solutionType <= 2 ? "grey" : "black"));
    lblIndicatorQ->setStyleSheet(lblIndicatorSolution->styleSheet());
    lblIndicatorQ->setToolTip(lblIndicatorSolution->toolTip());
    lblSolutionS->setText(lblSolution->text());
    lblSolutionS->setStyleSheet(lblSolution->styleSheet());
    lblSolutionQ->setText(ext + " " + label[0]->text() + " " + label[3]->text() + " " +
              label[1]->text() + " " + label[4]->text() + " " +
              label[2]->text() + " " + label[5]->text() + s[8]);
}
// update stream status indicators ------------------------------------------
void MainWindow::updateStream()
{
    QString color[] = {"red", "gray", "orange", "rgb(0,128,0)", "rgb(0,255,0)"};
    QLabel *ind[MAXSTRRTK] = {lblStream1, lblStream2, lblStream3, lblStream4, lblStream5, lblStream6, lblStream7, lblStream8};
    int i, sstat[MAXSTRRTK] = {0};
    char msg[MAXSTRMSG] = "";

    trace(4, "updateStream\n");

    rtksvrsstat(&rtksvr, sstat, msg);
    for (i = 0; i < MAXSTRRTK; i++) {
        ind[i]->setStyleSheet(QString("QLabel {background-color: %1}").arg(color[sstat[i] + 1]));
        if (sstat[i]) {
            lblMessage->setText(msg);
            lblMessage->setToolTip(msg);
        }
    }
}
// draw solution plot -------------------------------------------------------
void MainWindow::drawSolutionPlot(QLabel *plot, int type, int freq)
{
    QString s1, s2, fstr[NFREQ+2];;
    gtime_t time;
    int id;

    if (!plot) return;

    QPixmap buffer(plot->size());

    QPainter c(&buffer);
    QFont font;
    font.setPixelSize(8);
    c.setFont(font);

    if (plot==lblDisplay1) id=0;
    else if (plot==lblDisplay2) id=1;
    else if (plot==lblDisplay3) id=2;
    else id=3;

    int w = buffer.size().width() - 2;
    int h = buffer.height() - 2;
    int i, j, x, sat[2][MAXSAT], ns[2], snr[2][MAXSAT][NFREQ], vsat[2][MAXSAT];
    int *snr0[MAXSAT], *snr1[MAXSAT], topMargin = panelFont.pixelSize()*3/2;;
    double az[2][MAXSAT], el[2][MAXSAT], rr[3], pos[3];

    trace(4, "drawSolutionPlot\n");

    for (i = 0; i < NFREQ; i++) {
        fstr[i + 1] = QString("L%1").arg(i + 1);
    }
    fstr[i + 1] = " SYS";

    for (i = 0; i < MAXSAT; i++) {
        snr0[i] = snr[0][i];
        snr1[i] = snr[1][i];
    }
    ns[0] = rtksvrostat(&rtksvr, 0, &time, sat[0], az[0], el[0], snr0, vsat[0]);
    ns[1] = rtksvrostat(&rtksvr, 1, &time, sat[1], az[1], el[1], snr1, vsat[1]);

    rtksvrlock(&rtksvr);
    matcpy(rr, rtksvr.rtk.sol.rr, 3, 1);
    ecef2pos(rr, pos);
    rtksvrunlock(&rtksvr);

    for (i = 0; i < 2; i++) {
        if (ns[i] > 0) {
            numSatellites[i] = ns[i];
            for (int j = 0; j < ns[i]; j++) {
                satellites[i][j] = sat [i][j];
                satellitesAzimuth[i][j] = az[i][j];
                satellitesElevation[i][j] = el[i][j];
                for (int k = 0; k < NFREQ; k++)
                    satellitesSNR[i][j][k] = snr[i][j][k];
                validSatellites[i][j] = vsat[i][j];
            }
        } else {
            for (j = 0; j < numSatellites[i]; j++) {
                validSatellites[i][j] = 0;
                for (int k = 0; k < NFREQ; k++)
                    satellitesSNR[i][j][k] = 0;
            }
        }
    }
    c.fillRect(buffer.rect(), QBrush(Qt::white));
    x = 4;
    if (type == 0) { // snr plot rover+base
        if (w <= 3 * h) { // vertical
            drawSnr(c, w, (h - topMargin) / 2, 0, topMargin, 0, freq);
            drawSnr(c, w, (h - topMargin) / 2, 0, topMargin + (h - topMargin) / 2, 1, freq);
            s1 = QString(tr("Rover: Base %1 SNR (dBHz)")).arg(fstr[freq]);
            drawText(c, x, 1, s1, Qt::gray, 1, 2);
        } else { // horizontal
            drawSnr(c, w / 2, h - topMargin, 0, topMargin, 0, freq);
            drawSnr(c, w / 2, h - topMargin, w / 2, topMargin, 1, freq);
            s1 = QString(tr("Rover %1 SNR (dBHz)")).arg(fstr[freq]);
            s2 = QString(tr("Base %1 SNR (dBHz)")).arg(fstr[freq]);
            drawText(c, x, 1, s1, Qt::gray, 1, 2);
            drawText(c, w / 2 + x, 1, s2, Qt::gray, 1, 2);
        }
    } else if (type == 1) { // snr plot rover
        drawSnr(c, w, h - topMargin, 0, topMargin, 0, type);
        s1 = QString(tr("Rover %1 SNR (dBHz)")).arg(fstr[freq]);
        drawText(c, x, 1, s1, Qt::gray, 1, 2);
    } else if (type == 2) { // skyplot rover
        drawSatellites(c, w, h, 0, 0, 0, type);
        s1 = QString(tr("Rover %1")).arg(fstr[type]);
        drawText(c, x, 1, s1, Qt::gray, 1, 2);
    } else if (type == 3) { // skyplot+snr plot rover
        s1 = QString(tr("Rover %1")).arg(fstr[freq]);
        s2 = QString(tr("SNR (dBHz)"));
        if (w >= h * 2) { // horizontal
            drawSatellites(c, h, h, 0, 0, 0, freq);
            drawSnr(c, w - h, h - topMargin, h, topMargin, 0, freq);
            drawText(c, x, 1, s1, Qt::gray, 1, 2);
            drawText(c, x + h, 1, s2, Qt::gray, 1, 2);
        } else { // vertical
            drawSatellites(c, w, h / 2, 0, 0, 0, freq);
            drawSnr(c, w, (h - topMargin) / 2, 0, topMargin + (h - topMargin) / 2, 1, freq);
            drawText(c, x, 1, s1, Qt::gray, 1, 2);
        }
    } else if (type == 4) { // skyplot rover+base
        s1 = QString(tr("Rover %1")).arg(fstr[freq]);
        s2 = QString(tr("Base %1")).arg(fstr[freq]);
        if (w >= h) { // horizontal
            drawSatellites(c, w / 2, h, 0, 0, 0, freq);
            drawSatellites(c, w / 2, h, w / 2, 0, 1, freq);
            drawText(c, x, 1, s1, Qt::gray, 1, 2);
            drawText(c, x + w / 2, 1, s2, Qt::gray, 1, 2);
        } else { // vertical
            drawSatellites(c, w, h / 2, 0, 0, 0, freq);
            drawSatellites(c, w, h / 2, 0, h / 2, 1, freq);
            drawText(c, x, 1, s1, Qt::gray, 1, 2);
            drawText(c, x, h / 2 + 1, s2, Qt::gray, 1, 2);
        }
    } else if (type == 5) { // baseline plot
        drawBaseline(c, id, w, h);
        drawText(c, x, 1, tr("Baseline"), Qt::gray, 1, 2);
    } else if (type == 6) { // track plot
        drawTrack(c, id, &buffer);
        drawText(c, x, 3, tr("Gnd Trk"), Qt::gray, 1, 2);
    }
    plot->setPixmap(buffer);
}
// update solution plot ------------------------------------------------------
void MainWindow::updatePlot()
{
    if (panel22->isVisible()) {
        drawSolutionPlot(lblDisplay1, plotType[0], frequencyType[0]);
    }
    if (panel23->isVisible()) {
        drawSolutionPlot(lblDisplay2, plotType[1], frequencyType[1]);
    }
    if (panel24->isVisible()) {
        drawSolutionPlot(lblDisplay3, plotType[2], frequencyType[2]);
    }
    if (panel25->isVisible()) {
        drawSolutionPlot(lblDisplay4, plotType[3], frequencyType[3]);
    }
}
// snr color ----------------------------------------------------------------
QColor MainWindow::snrColor(int snr)
{
    QColor color[] = {Qt::green, QColor(255, 128, 0, 255), QColor(0, 255, 128, 255), Qt::blue, Qt::red, Qt::gray};
    uint32_t r1, g1, b1;
    QColor c1, c2;
    double a;
    int i;

    if (snr < 25) return color[5];
    if (snr < 27) return color[4];
    if (snr > 47) return color[0];
    a = (snr - 27.5) / 5.0;
    i = static_cast<int>(a);
    a -= i;
    c1 = color[3 - i];
    c2 = color[4 - i];
    r1 = static_cast<uint32_t>(a * c1.red() + (1.0 - a) * c2.red()) & 0xFF;
    g1 = static_cast<uint32_t>(a * c1.green() + (1.0 - a) * c2.green()) & 0xFF;
    b1 = static_cast<uint32_t>(a * c1.blue() + (1.0 - a) * c2.blue()) & 0xFF;

    return QColor(r1, g1, b1);
}
// draw snr plot ------------------------------------------------------------
void MainWindow::drawSnr(QPainter &c, int w, int h, int x0, int y0,
             int index, int freq)
{
    static const QColor color[] = {
        QColor(0,   128, 0), QColor(0, 128, 128), QColor(0xA0,   0, 0xA0),
        QColor(128, 0,	 0), QColor(0, 0,   128), QColor( 128, 128,    0),
        QColor(128, 128, 128)
    };
    static const QColor color_sys[] = {
        Qt::green, ColorOrange, ColorFuchsia, Qt::blue, Qt::red, Qt::darkCyan, Qt::gray
    };
    QString s;
    int i, j, snrIdx, sysIdx, numSystems, x1, y1, height, offset, topMargin, bottomMargin, hh, barDistance, barWidth, snr[NFREQ + 1], sysMask[7] = {0};
    char id[16], sys[] = "GREJCS", *q;

    trace(4, "drawSnr: w=%d h=%d x0=%d y0=%d index=%d freq=%d\n", w, h, x0, y0, index, freq);

    QFontMetrics fm(panelFont);
    topMargin = fm.height();
    bottomMargin = fm.height();
    y0 += topMargin;
    hh = h - topMargin - bottomMargin;

    // draw horizontal lines
    c.setPen(ColorSilver);
    for (snr[0] = MINSNR + 10; snr[0] < MAXSNR; snr[0] += 10) {
        y1 = y0 + hh - (snr[0] - MINSNR) * hh / (MAXSNR - MINSNR);
        c.drawLine(x0 + 2, y1, x0 + w - 20, y1);
        drawText(c, x0 + w - 4, y1, QString::number(snr[0]), Qt::gray, 2, 0);
    }

    // draw outer box
    y1 = y0 + hh;
    QRect b(x0 + 2, y0, w - 2, hh);
    c.setBrush(Qt::NoBrush);
    c.setPen(Qt::gray);
    c.drawRect(b);

    for (i = 0; i < numSatellites[index] && i < MAXSAT; i++) {
        barDistance = (w - 16) / numSatellites[index];
        barWidth = barDistance - 2 < 8 ? barDistance - 2 : 8;
        x1 = x0 + i * barDistance + barDistance / 2;
        satno2id(satellites[index][i], id);
        sysIdx = (q = strchr(sys, id[0])) ? (int)(q - sys) : 6;

        for (j = snr[0] = 0; j < NFREQ; j++) {
            snr[j + 1] = satellitesSNR[index][i][j];
            if ((freq && freq == j + 1) || ((!freq || freq > NFREQ) && snr[j + 1] > snr[0]))
                snr[0] = snr[j + 1];  // store max snr
        }
        for (j = 0; j < NFREQ + 2; j++) {
            snrIdx = j < NFREQ + 1 ? j : 0;
            offset = j < NFREQ + 1 ? 0 : 2;
            if (snr[snrIdx] > 0) height = (snr[snrIdx] - MINSNR) * hh / (MAXSNR - MINSNR);
            else height = offset;
            height = height > y1 - 2 ? y1 - 2 : (height < 0 ? 0 : height);  // limit bar not go negative or to high

            QRect r1(x1, y1, barWidth, height);
            if (j == 0) {  // filled bar
                c.setBrush(QBrush(freq < NFREQ + 1 ? snrColor(snr[snrIdx]) : color_sys[sysIdx], Qt::SolidPattern));
                if (!validSatellites[index][i])
                    c.setBrush(QBrush(QColor(0x0c, 0x0c, 0x0c), Qt::SolidPattern));
            } else {  // outline only
                c.setPen(j < NFREQ + 1 ? QColor(0x0c, 0x0c, 0x0c) : Qt::gray);
                c.setBrush(Qt::NoBrush);
            }
            c.drawRect(r1);
        }
        // draw satellite id
        drawText(c, x1 + barWidth / 2, y1, (s = id + 1), color[sysIdx], 0, 2);
        sysMask[sysIdx] = 1;
    }
    for (i = numSystems = 0; i < 7; i++)
        if (sysMask[i]) numSystems++;

    // draw indicators for used navigation systems
    for (i = j = 0; i < 7; i++) {
        if (!sysMask[i]) continue;
        sprintf(id, "%c", sys[i]);
        drawText(c, x0 + w - topMargin*3/2 + fm.averageCharWidth()*9/8 * (j++ - numSystems), y0 + topMargin, (s = id), color[i], 0, 2);
    }
}
// draw satellites in skyplot -----------------------------------------------
void MainWindow::drawSatellites(QPainter &c, int w, int h, int x0, int y0,
             int index, int freq)
{
    static const QColor color_sys[] = {
        Qt::green, ColorOrange, ColorFuchsia, Qt::blue, Qt::red, Qt::darkCyan, Qt::gray
    };
    QColor color_text;
    QPoint p(w / 2, h / 2);
    double r = MIN(w * 0.95, h * 0.95) / 2, azel[MAXSAT * 2], dop[4];
    int i, j, k, sysIdx, radius, x[MAXSAT], y[MAXSAT], snr[NFREQ+1], nsats = 0;
    char id[16], sys[] = "GREJCIS", *q;

    trace(4, "drawSatellites: w=%d h=%d index=%d freq=%d\n", w, h, index, freq);

    drawSky(c, w, h, x0, y0);  // draw background

    // draw satellites
    for (i = 0, k = numSatellites[index] - 1; i < numSatellites[index] && i < MAXSAT; i++, k--) {
        if (satellitesElevation[index][k] <= 0.0) continue;
        for (j = snr[0] = 0; j < NFREQ; j++) {
            snr[j + 1] = satellitesSNR[index][k][j];
            if ((freq && freq == j + 1) || ((!freq || freq > NFREQ) && snr[j + 1] > snr[0])) {
                snr[0] = snr[j + 1]; // max snr
            }
        }
        if (validSatellites[index][k] && (snr[freq] > 0 || freq > NFREQ)) {
            azel[nsats * 2] = satellitesAzimuth[index][k];
            azel[nsats * 2 + 1] = satellitesElevation[index][k];
            nsats++;
        }
        satno2id(satellites[index][k], id);
        sysIdx = (q = strchr(sys, id[0])) ? (int)(q - sys) : 6;
        x[i] = static_cast<int>(p.x() + r * (90 - satellitesElevation[index][k] * R2D) / 90 * sin(satellitesAzimuth[index][k])) + x0;
        y[i] = static_cast<int>(p.y() - r * (90 - satellitesElevation[index][k] * R2D) / 90 * cos(satellitesAzimuth[index][k])) + y0;
        radius = panelFont.pixelSize() * 3 / 2;

        c.setBrush(!validSatellites[index][k] ? ColorSilver :
                        (freq < NFREQ ? snrColor(snr[freq]) : color_sys[sysIdx]));
        c.setPen(Qt::gray);
        color_text = Qt::white;
        if (freq < NFREQ + 1 && snr[freq] <= 0) {
            c.setPen(ColorSilver);
            color_text = ColorSilver;
        }
        // draw satllites
        c.drawEllipse(x[i] - radius, y[i] - radius, 2 * radius + 1, 2 * radius + 1);
        drawText(c, x[i], y[i], id, color_text, 0, 0);
    }

    // draw annotations
    dops(nsats, azel, 0.0, dop);
    drawText(c, x0 + 3, y0 + h, QString(tr("# Sat: %1/%2")).arg(nsats,2).arg(numSatellites[index], 2), Qt::gray, 1, 1);
    drawText(c, x0 + w - 3, y0 + h, QString(tr("GDOP: %1")).arg(dop[0], 0, 'f', 1), Qt::gray, 2, 1);
}
// draw baseline plot -------------------------------------------------------
void MainWindow::drawBaseline(QPainter &c, int id, int w, int h)
{
    QColor colors[] = {ColorSilver, Qt::green, ColorOrange, ColorFuchsia, Qt::blue, Qt::red, ColorTeal};
    QString directions[] = {tr("N"), tr("E"), tr("S"), tr("W")};
    QPoint center(w / 2, h / 2), p1, p2, pp;
    double radius = MIN(w * 0.95, h * 0.95) / 2;
    double *rrover = solutionRover + solutionsCurrent * 3;
    double *rbase = solutionReference + solutionsCurrent * 3;
    double baseline[3] = {0}, pos[3], enu[3], len = 0.0, pitch = 0.0, yaw = 0.0;
    double cp, q, az = 0.0;
    QColor col = Qt::white;
    int i, d1 = 10, d2 = 16, d3 = 10, cy = 0, sy = 0, cya = 0, sya = 0, a, x1, x2, y1, y2, radius2, digit;

    trace(4, "drawBaseline: w=%d h=%d\n", w, h);

    if (PMODE_DGPS <= processingOptions.mode && processingOptions.mode <= PMODE_FIXED) {
        col = (rtksvr.state && solutionStatus[solutionsCurrent] && solutionCurrentStatus) ? colors[solutionStatus[solutionsCurrent]] : Qt::white;

        if (norm(rrover, 3) > 0.0 && norm(rbase, 3) > 0.0)
            for (i = 0; i < 3; i++)
                baseline[i] = rrover[i] - rbase[i];

        if ((len = norm(baseline, 3)) > 0.0) {
            ecef2pos(rbase, pos); ecef2enu(pos, baseline, enu);
            pitch = asin(enu[2] / len);
            yaw = atan2(enu[0], enu[1]);
            if (yaw < 0.0) yaw += 2.0 * PI;
            if (baselineMode[id]) az = yaw;
        }
    }
    if (len >= MIN_BASELINE_LEN) {
        cp = cos(pitch);
        cy = static_cast<int>((radius - d1 - d2 / 2) * cp * cos(yaw - az));
        sy = static_cast<int>((radius - d1 - d2 / 2) * cp * sin(yaw - az));
        cya = static_cast<int>(((radius - d1 - d2 / 2) * cp - d2 / 2 - 4) * cos(yaw - az));
        sya = static_cast<int>(((radius - d1 - d2 / 2) * cp - d2 / 2 - 4) * sin(yaw - az));
    }
    p1 = QPoint(center.x() - sy, center.y() + cy);       // base
    p2 = QPoint(center.x() + sy, center.y() - cy);       // rover

    // draw two outer circles
    c.setPen(Qt::gray);
    c.drawEllipse(center.x() - radius, center.y() - radius, 2 * radius + 1, 2 * radius + 1);
    radius2 = static_cast<int>(radius - d1 / 2);
    c.drawEllipse(center.x() - radius2, center.y() - radius2, 2 * radius2 + 1, 2 * radius2 + 1);
    for (a = 0; a < 360; a += 5) {  // draw ticks
        q = a % 90 == 0 ? 0 : (a % 30 == 0 ? radius - d1 * 3 : (a % 10 == 0 ? radius - d1 * 2 : radius - d1));
        x1 = static_cast<int>(radius * sin(a * D2R - az));
        y1 = static_cast<int>(radius * cos(a * D2R - az));
        x2 = static_cast<int>(q * sin(a * D2R - az));
        y2 = static_cast<int>(q * cos(a * D2R - az));
        c.setPen(ColorSilver);
        c.drawLine(center.x() + x1, center.y() - y1, center.x() + x2, center.y() - y2);
        c.setBrush(Qt::white);
        if (a % 90 == 0)  // draw label
            drawText(c, center.x() + x1, center.y() - y1, directions[a / 90], Qt::gray, 0, 0);
        if (a == 0) {  // raw arrow to north
            x1 = static_cast<int>((radius - d1 * 3 / 2) * sin(a * D2R - az));
            y1 = static_cast<int>((radius - d1 * 3 / 2) * cos(a * D2R - az));
            drawArrow(c, center.x() + x1, center.y() - y1, d3, -static_cast<int>(az * R2D), ColorSilver);
        }
    }

    pp = pitch < 0.0 ? p2 : p1;
    c.setPen(ColorSilver);
    c.drawLine(center, pp);
    if (pitch < 0.0) {
        c.setBrush(Qt::white);
        c.drawEllipse(pp.x() - d2 / 2, pp.y() - d2 / 2, d2 + 1, d2 + 1);
        drawArrow(c, center.x() + sya, center.y() - cya, d3, static_cast<int>((yaw - az) * R2D), ColorSilver);
    }
    c.setBrush(col);
    c.drawEllipse(pp.x() - d2 / 2 + 2, pp.y() - d2 / 2 + 2, d2 - 1, d2 - 1);

    pp = pitch >= 0.0 ? p2 : p1;
    c.setPen(Qt::gray);
    c.drawLine(center, pp);
    if (pitch >= 0.0) {
        c.setBrush(Qt::white);
        c.drawEllipse(pp.x() - d2 / 2, pp.y() - d2 / 2, d2, d2);
        drawArrow(c, center.x() + sya, center.y() - cya, d3, static_cast<int>((yaw - az) * R2D), Qt::gray);
    }
    c.setBrush(col);
    c.drawEllipse(pp.x() - d2 / 2 + 2, pp.y() - d2 / 2 + 2, d2 - 4, d2 - 4);

    // draw annotations
    c.setBrush(Qt::white);
    digit = len < 1000.0 ? 3 : (len < 10000.0 ? 2 : (len < 100000.0 ? 1 : 0));
    drawText(c, center.x(), center.y(), QString("%1 m").arg(len, 0, 'f', digit), Qt::gray, 0, 0);
    drawText(c, 3, h, QString("Y: %1%2").arg(yaw * R2D, 0, 'f', 1).arg(degreeChar), Qt::gray, 1, 1);
    drawText(c, w - 3, h, QString("P: %1%2").arg(pitch * R2D, 0, 'f', 1).arg(degreeChar), Qt::gray, 2, 1);
}
// draw track plot ----------------------------------------------------------
void MainWindow::drawTrack(QPainter &c, int id, QPaintDevice *plot)
{
    QColor mcolor[] = {ColorSilver, Qt::green, ColorOrange, ColorFuchsia, Qt::blue, Qt::red, ColorTeal};
    QColor *color;
    Graph *graph = new Graph(plot);
    QPoint p1, p2;
    QString label;
    double scale[] = {
        0.00021, 0.00047, 0.001, 0.0021, 0.0047, 0.01, 0.021, 0.047, 0.1,   0.21,   0.47,
        1.0,	 2.1,	  4.7,	 10.0,	 21.0,	 47.0, 100.0, 210.0, 470.0, 1000.0, 2100.0,4700.0,
        10000.0
    };
    double *x, *y, xt, yt, sx, sy, ref[3], pos[3], dr[3], enu[3];
    int i, j, currentPointNo, numPoints = 0, type, scl;

    trace(3, "DrawTrk\n");

    type = id == 0 ? trackType[0] : trackType[1];
    scl = id == 0 ? trackScale[0] : trackScale[1];

    x = new double[solutionBufferSize];
    y = new double[solutionBufferSize];
    color = new QColor[solutionBufferSize];

    if (norm(trackOrigin, 3) < 1E-6) {
        if (norm(solutionReference + solutionsCurrent * 3, 3) > 1E-6)
            matcpy(trackOrigin, solutionReference + solutionsCurrent * 3, 3, 1);
        else
            matcpy(trackOrigin, solutionRover + solutionsCurrent * 3, 3, 1);
    }
    if (norm(solutionReference + solutionsCurrent * 3, 3) > 1E-6)
        matcpy(ref, solutionReference + solutionsCurrent * 3, 3, 1);
    else
        matcpy(ref, trackOrigin, 3, 1);
    ecef2pos(ref, pos);

    for (i = currentPointNo = solutionsStart; i != solutionsEnd; ) {
        for (j = 0; j < 3; j++) dr[j] = solutionRover[j + i * 3] - ref[j];
        if (i == solutionsCurrent) currentPointNo = numPoints;
        ecef2enu(pos, dr, enu);
        x[numPoints] = enu[0];
        y[numPoints] = enu[1];
        color[numPoints] = mcolor[solutionStatus[i]];
        numPoints++;
        if (++i >= solutionBufferSize) i = 0;
    }
    graph->setSize(plot->width(), plot->height());
    graph->setScale(scale[scl], scale[scl]);
    graph->color[1] = ColorSilver;

    // center to current position
    if (numPoints > 0)
        graph->setCenter(x[currentPointNo], y[currentPointNo]);

    if (type == 1) {
        graph->xLabelPosition = Graph::LabelPosition::Axis;
        graph->yLabelPosition = Graph::LabelPosition::Axis;
        graph->drawCircles(c, 0);
    } else {
        graph->xLabelPosition = Graph::LabelPosition::Inner;
        graph->yLabelPosition = Graph::LabelPosition::InnerRot;
        graph->drawAxis(c, 0, 0);
    }
    graph->drawPoly(c, x, y, numPoints, ColorSilver, 0); // draw lines through all points
    graph->drawMarks(c, x, y, color, numPoints, 0, 3, 0); // draw markers

    // highlight current position
    if (numPoints > 0) {
        graph->toPoint(x[currentPointNo], y[currentPointNo], p1);
        graph->drawMark(c, p1, Graph::MarkerTypes::Dot, Qt::white, 18, 0);
        graph->drawMark(c, p1, Graph::MarkerTypes::Circle, rtksvr.state ? Qt::black : Qt::gray, 16, 0);
        graph->drawMark(c, p1, Graph::MarkerTypes::Plus, rtksvr.state ? Qt::black : Qt::gray, 20, 0);
        graph->drawMark(c, p1, Graph::MarkerTypes::Dot, rtksvr.state ? Qt::black : Qt::gray, 12, 0);
        graph->drawMark(c, p1, Graph::MarkerTypes::Dot, rtksvr.state ? color[currentPointNo] : Qt::white, 10, 0);
    }

    // draw scale
    graph->getPosition(p1, p2);
    graph->getTick(xt, yt);
    graph->getScale(sx, sy);
    p2.rx() -= 35;
    p2.ry() -= 12;
    graph->drawMark(c, p2, Graph::MarkerTypes::HScale, Qt::gray, static_cast<int>(xt / sx + 0.5), 0);

    // scale text
    p2.ry() -= 2;
    if (xt < 0.01)
        label = QString("%1 mm").arg(xt * 1000.0, 0, 'f', 0);
    else if (xt < 1.0)
        label = QString("%1 cm").arg(xt * 100.0, 0, 'f', 0);
    else if (xt < 1000.0)
        label = QString("%1 m").arg(xt, 0, 'f', 0);
    else
        label = QString("%1 km").arg(xt / 1000.0, 0, 'f', 0);
    graph->drawText(c, p2, label, Qt::gray, Qt::white, 0, 1, 0);

    // reference position
    if (norm(ref, 3) > 1E-6) {
        p1.rx() += 2;
        p1.ry() = p2.y() + 11;
        label = QString("%1° %2°").arg(pos[0] * R2D, 0, 'f', 9).arg(pos[1] * R2D, 0, 'f', 9);
        graph->drawText(c, p1, label, Qt::gray, Qt::white, 1, 1, 0);
    }

    delete graph;
    delete[] x;
    delete[] y;
    delete[] color;
}
// draw skyplot -------------------------------------------------------------
void MainWindow::drawSky(QPainter &c, int w, int h, int x0, int y0)
{
    QString label[] = {tr("N"), tr("E"), tr("S"), tr("W")};
    QPoint p(x0 + w / 2, y0 + h / 2);
    double radius = MIN(w * 0.95, h * 0.95) / 2;
    int a, e, d, x, y;

    c.setBrush(Qt::white);
    // draw elevation circles every 30 deg
    for (e = 0; e < 90; e += 30) {
        d = static_cast<int>(radius * (90 - e) / 90);
        c.setPen(e == 0 ? Qt::gray : ColorSilver);  // outline in gray, other lines in silver
        c.drawEllipse(p.x() - d, p.y() - d, 2 * d + 1, 2 * d + 1);
    }
    // draw azimuth lines from center outwards every 45 deg
    for (a = 0; a < 360; a += 45) {
        x = static_cast<int>(radius * sin(a * D2R));
        y = static_cast<int>(radius * cos(a * D2R));
        c.setPen(ColorSilver);
        c.drawLine(p.x(), p.y(), p.x() + x, p.y() - y);
        if (a % 90 == 0)  // draw labels every 90 deg
            drawText(c, p.x() + x, p.y() - y, label[a / 90], Qt::gray, 0, 0);
    }
}
// draw text ----------------------------------------------------------------
void MainWindow::drawText(QPainter &c, int x, int y, const QString &s,
              const QColor &color, int ha, int va)
{
    // ha  = horizontal alignment (0: center, 1: left,   2: right)
    // va  = vertical alignment   (0: center, 1: bottom, 2: top  )
    int flags = 0;

    switch (ha) {
        case 0: flags |= Qt::AlignHCenter; break;
        case 1: flags |= Qt::AlignLeft; break;
        case 2: flags |= Qt::AlignRight; break;
    }
    switch (va) {
        case 0: flags |= Qt::AlignVCenter; break;
        case 1: flags |= Qt::AlignBottom; break;
        case 2: flags |= Qt::AlignTop; break;
    }
    c.setFont(panelFont);
    c.setPen(color);

    QRectF off = c.boundingRect(QRectF(), flags, s);

    c.translate(x, y);
    c.drawText(off, s);
    c.translate(-x, -y);
}
// draw arrow ---------------------------------------------------------------
void MainWindow::drawArrow(QPainter &c, int x, int y, int siz,
               int ang, const QColor &color)
{
    QPoint p1[4], p2[4];
    int i;

    // create arrow head
    p1[0] = QPoint(0, siz / 2);
    p1[1] = QPoint(siz / 2, -siz / 2);
    p1[2] = QPoint(-siz / 2, -siz / 2);
    p1[3] = QPoint(0, siz / 2);

    // rotate points
    for (i = 0; i < 4; i++) {
        p2[i] = QPoint(x + static_cast<int>(p1[i].x() * cos(-ang * D2R) - p1[i].y() * sin(-ang * D2R) + 0.5),
                       y - static_cast<int>(p1[i].x() * sin(-ang * D2R) + p1[i].y() * cos(-ang * D2R) + 0.5));
    }
    c.setBrush(QBrush(color, Qt::SolidPattern));
    c.setPen(color);
    c.drawPolygon(p2, 4);
}
// open monitor port --------------------------------------------------------
void MainWindow::openMonitorPort(int port)
{
    QString s;
    int i;

    if (port <= 0) return;

    trace(3, "openMonitorPort: port=%d\n", port);

    for (i = 0; i <= MAX_PORT_OFFSET; i++) {
        if (stropen(&monistr, STR_TCPSVR, STR_MODE_RW, qPrintable(QString(":%1").arg(port + i)))) {
            strsettimeout(&monistr, timeoutTime, reconnectionTime);
            if (i > 0)
                setWindowTitle(QString(tr("%1 ver.%2 (%3)")).arg(PRGNAME).arg(VER_RTKLIB).arg(i + 1));
            monitorPortOpen = port + i;
            return;
        }
    }
    QMessageBox::critical(this, tr("Error"), QString(tr("Could not open any monitor port betwenn %1-%2")).arg(port).arg(port + MAX_PORT_OFFSET));
    monitorPortOpen = 0;
}
// initialize solution buffer -----------------------------------------------
void MainWindow::initSolutionBuffer()
{
    double ep[] = { 2000, 1, 1, 0, 0, 0 };
    int i, j;

    trace(3, "InitSolBuff\n");

    delete [] timeStamps;
    delete [] solutionStatus;
    delete [] numValidSatellites;
    delete [] solutionRover;
    delete [] solutionReference;
    delete [] solutionQr;
    delete [] velocityRover;
    delete [] ages;
    delete [] ratioAR;

    if (solutionBufferSize <= 0) solutionBufferSize = 1;

    timeStamps = new gtime_t[solutionBufferSize];
    solutionStatus = new int[solutionBufferSize];
    numValidSatellites = new int[solutionBufferSize];
    solutionRover = new double[solutionBufferSize * 3];
    solutionReference = new double[solutionBufferSize * 3];
    velocityRover = new double[solutionBufferSize * 3];
    solutionQr = new double[solutionBufferSize * 9];
    ages = new double[solutionBufferSize];
    ratioAR = new double[solutionBufferSize];

    solutionsCurrent = solutionsStart = solutionsEnd = 0;

    for (i = 0; i < solutionBufferSize; i++) {
        timeStamps[i] = epoch2time(ep);
        solutionStatus[i] = numValidSatellites[i] = 0;
        for (j = 0; j < 3; j++) solutionRover[j + i * 3] = solutionReference[j + i * 3] = velocityRover[j + i * 3] = 0.0;
        for (j = 0; j < 9; j++) solutionQr[j + i * 9] = 0.0;
        ages[i] = ratioAR[i] = 0.0;
    }

    sBSolution->setMaximum(0); sBSolution->setValue(0);
}
// save log file ------------------------------------------------------------
void MainWindow::saveLogs()
{
    QString fileName;
    int posf[] = {SOLF_LLH, SOLF_LLH, SOLF_XYZ, SOLF_ENU, SOLF_ENU, SOLF_LLH};
    solopt_t opt;
    sol_t sol = {};
    double ep[6], pos[3];
    QString fileTemplate;
    int i;

    trace(3, "saveLogs\n");

    time2epoch(timeget(), ep);
    fileTemplate = QString("rtk_%1%2%3%4%5%6.txt")
               .arg(ep[0], 4, 'f', 0, QChar('0')).arg(ep[1], 2, 'f', 0, QChar('0')).arg(ep[2], 2, 'f', 0, QChar('0'))
               .arg(ep[3], 2, 'f', 0, QChar('0')).arg(ep[4], 2, 'f', 0, QChar('0')).arg(ep[5], 2, 'f', 0, QChar('0'));
    fileName = QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, QString(), fileTemplate));
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open log file: %1").arg(fileName));
        return;
    }
    QTextStream str(&file);

    opt = solutionOptions;
    opt.posf = posf[solutionType];
    if (solutionOptions.outhead) {
        QString data;

        data = QString(tr("%% program   : %1 ver.%2\n")).arg(PRGNAME).arg(VER_RTKLIB);
        str << data;
        if (processingOptions.mode == PMODE_DGPS || processingOptions.mode == PMODE_KINEMA ||
            processingOptions.mode == PMODE_STATIC) {
            ecef2pos(processingOptions.rb, pos);
            data = QString(tr("%% ref pos   :%1 %2 %3\n")).arg(pos[0] * R2D, 13, 'f', 9)
                   .arg(pos[1] * R2D, 14, 'f', 9).arg(pos[2], 10, 'f', 4);
            str << data;
        }
        file.write("%%\n");
    }
    FILE *f = fdopen(file.handle(), "w");
    outsolhead(f, &opt);

    for (i = solutionsStart; i != solutionsEnd; ) {
        sol.time = timeStamps[i];
        matcpy(sol.rr, solutionRover + i * 3, 3, 1);
        sol.stat = solutionStatus[i];
        sol.ns = numValidSatellites[i];
        sol.ratio = ratioAR[i];
        sol.age = ages[i];
        outsol(f, &sol, solutionReference + i * 3, &opt);
        if (++i >= solutionBufferSize) i = 0;
    }
}
// load navigation data -----------------------------------------------------
void MainWindow::loadNavigation(nav_t *nav)
{
    QSettings settings(iniFile, QSettings::IniFormat);
    QString str;
    eph_t eph0;
    char buff[2049], *p;
    long toe_time,toc_time,ttr_time;
    int i;

    trace(3, "loadNavigation\n");

    memset(&eph0, 0, sizeof(eph_t));

    for (i = 0; i < 2 * MAXSAT; i++) {
        if ((str = settings.value(QString("navi/eph_%1").arg(i, 2)).toString()).isEmpty()) continue;
        nav->eph[i] = eph0;
        strncpy(buff, qPrintable(str), 2048);
        if (!(p = strchr(buff, ','))) continue;
        *p = '\0';
        if (!(nav->eph[i].sat = satid2no(buff))) continue;
        sscanf(p + 1, "%d,%d,%d,%d,%ld,%ld,%ld,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%lf",
               &nav->eph[i].iode,
               &nav->eph[i].iodc,
               &nav->eph[i].sva,
               &nav->eph[i].svh,
               &toe_time,
               &toc_time,
               &ttr_time,
               &nav->eph[i].A,
               &nav->eph[i].e,
               &nav->eph[i].i0,
               &nav->eph[i].OMG0,
               &nav->eph[i].omg,
               &nav->eph[i].M0,
               &nav->eph[i].deln,
               &nav->eph[i].OMGd,
               &nav->eph[i].idot,
               &nav->eph[i].crc,
               &nav->eph[i].crs,
               &nav->eph[i].cuc,
               &nav->eph[i].cus,
               &nav->eph[i].cic,
               &nav->eph[i].cis,
               &nav->eph[i].toes,
               &nav->eph[i].fit,
               &nav->eph[i].f0,
               &nav->eph[i].f1,
               &nav->eph[i].f2,
               &nav->eph[i].tgd[0],
               &nav->eph[i].code,
               &nav->eph[i].flag,
               &nav->eph[i].tgd[1]);
        nav->eph[i].toe.time = toe_time;
        nav->eph[i].toc.time = toc_time;
        nav->eph[i].ttr.time = ttr_time;
    }
    // read ionospheric parameters
    str = settings.value("navi/ion", "").toString();
    QStringList tokens = str.split(",");
    for (i = 0; i < 8; i++) nav->ion_gps[i] = 0.0;
    for (i = 0; (i < 8) && (i < tokens.size()); i++) nav->ion_gps[i] = tokens.at(i).toDouble();

    // read time offsets
    str = settings.value("navi/utc", "").toString();
    tokens = str.split(",");
    for (i = 0; i < 8; i++) nav->utc_gps[i] = 0.0;
    for (i = 0; (i < 8) && (i < tokens.size()); i++) nav->utc_gps[i] = tokens.at(i).toDouble();
}
// save navigation data -----------------------------------------------------
void MainWindow::saveNavigation(nav_t *nav)
{
    QSettings settings(iniFile, QSettings::IniFormat);
    QString str;
    char id[32];
    int i;

    trace(3, "saveNavigation\n");

    if (nav == NULL) return;

    for (i = 0; i < MAXSAT*2; i++) {
        if (nav->eph[i].ttr.time == 0) continue;
        str = "";
        satno2id(nav->eph[i].sat, id);
        str = str + id + ",";
        str = str + QString("%1,").arg(nav->eph[i].iode);
        str = str + QString("%1,").arg(nav->eph[i].iodc);
        str = str + QString("%1,").arg(nav->eph[i].sva);
        str = str + QString("%1,").arg(nav->eph[i].svh);
        str = str + QString("%1,").arg(static_cast<int>(nav->eph[i].toe.time));
        str = str + QString("%1,").arg(static_cast<int>(nav->eph[i].toc.time));
        str = str + QString("%1,").arg(static_cast<int>(nav->eph[i].ttr.time));
        str = str + QString("%1,").arg(nav->eph[i].A, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].e, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].i0, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].OMG0, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].omg, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].M0, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].deln, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].OMGd, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].idot, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].crc, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].crs, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].cuc, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].cus, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].cic, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].cis, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].toes, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].fit, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].f0, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].f1, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].f2, 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].tgd[0], 0, 'E', 14);
        str = str + QString("%1,").arg(nav->eph[i].code);
        str = str + QString("%1,").arg(nav->eph[i].flag);
        str = str + QString("%1,").arg(nav->eph[i].tgd[1],0,'E',14);
        settings.setValue(QString("navi/eph_%1").arg(i, 2), str);
    }
    str = "";
    for (i = 0; i < 8; i++) str = str + QString("%1,").arg(nav->ion_gps[i], 0, 'E', 14);
    settings.setValue("navi/ion", str);

    str = "";
    for (i = 0; i < 4; i++) str = str + QString("%1,").arg(nav->utc_gps[i], 0, 'E', 14);
    settings.setValue("navi/utc", str);
}
// set tray icon ------------------------------------------------------------
void MainWindow::setTrayIcon(int index)
{
    QPixmap pix(":/buttons/navi");

    systemTray->setIcon(QIcon(pix.copy(index * 16, 0, 16, 16)));
}
// load option from ini file ------------------------------------------------
void MainWindow::loadOptions()
{
    QSettings settings(iniFile, QSettings::IniFormat);
    QString s;
    int i, j, no, strno[] = { 0, 1, 6, 2, 3, 4, 5, 7 };

    trace(3, "LoadOpt\n");

    for (i = 0; i < 8; i++) {
        no = strno[i];
        streamEnabled[i] = settings.value(QString("stream/streamc%1").arg(no), 0).toInt();
        streamType[i] = settings.value(QString("stream/stream%1").arg(no), 0).toInt();
        inputFormat[i] = settings.value(QString("stream/format%1").arg(no), 0).toInt();
        for (j = 0; j < 4; j++)
            paths[i][j] = settings.value(QString("stream/path_%1_%2").arg(no).arg(j), "").toString();
    }

    for (i = 0; i < 3; i++)
        receiverOptions [i] = settings.value(QString("stream/rcvopt%1").arg(i + 1), "").toString();

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
            commands[i][j] = settings.value(QString("serial/cmd_%1_%2").arg(i).arg(j), "").toString();
            commandEnabled[i][j] = settings.value(QString("serial/cmdena_%1_%2").arg(i).arg(j), 0).toInt();
            commands[i][j].replace("@@", "\r\n");
        }

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
            commandsTcp[i][j] = settings.value(QString("tcpip/cmd_%1_%2").arg(i).arg(j), "").toString();
            commandEnableTcp[i][j] = settings.value(QString("tcpip/cmdena_%1_%2").arg(i).arg(j), 0).toInt();
            commandsTcp[i][j].replace("@@", "\r\n");
        }

    processingOptions.mode = settings.value("prcopt/mode", 0).toInt();
    processingOptions.nf = settings.value("prcopt/nf", 2).toInt();
    processingOptions.elmin = settings.value("prcopt/elmin", 15.0 * D2R).toFloat();
    processingOptions.snrmask.ena[0] = settings.value("prcopt/snrmask_ena1", 0).toInt();
    processingOptions.snrmask.ena[1] = settings.value("prcopt/snrmask_ena2", 0).toInt();
    for (i = 0; i < NFREQ; i++) for (j = 0; j < 9; j++)
            processingOptions.snrmask.mask[i][j] =
                settings.value(QString("prcopt/snrmask_%1_%2").arg(i + 1).arg(j + 1), 0.0).toInt();

    processingOptions.dynamics = settings.value("prcopt/dynamics", 0).toInt();
    processingOptions.tidecorr = settings.value("prcopt/tidecorr", 0).toInt();
    processingOptions.modear = settings.value("prcopt/modear", 1).toInt();
    processingOptions.glomodear = settings.value("prcopt/glomodear", 0).toInt();
    processingOptions.bdsmodear = settings.value("prcopt/bdsmodear", 0).toInt();
    processingOptions.maxout = settings.value("prcopt/maxout", 5).toInt();
    processingOptions.minlock = settings.value("prcopt/minlock", 0).toInt();
    processingOptions.minfix = settings.value("prcopt/minfix", 10).toInt();
    processingOptions.ionoopt = settings.value("prcopt/ionoopt", IONOOPT_BRDC).toInt();
    processingOptions.tropopt = settings.value("prcopt/tropopt", TROPOPT_SAAS).toInt();
    processingOptions.sateph = settings.value("prcopt/ephopt", EPHOPT_BRDC).toInt();
    processingOptions.niter = settings.value("prcopt/niter", 1).toInt();
    processingOptions.eratio[0] = settings.value("prcopt/eratio0", 100.0).toDouble();
    processingOptions.eratio[1] = settings.value("prcopt/eratio1", 100.0).toDouble();
    processingOptions.err[1] = settings.value("prcopt/err1", 0.003).toDouble();
    processingOptions.err[2] = settings.value("prcopt/err2", 0.003).toDouble();
    processingOptions.err[3] = settings.value("prcopt/err3", 0.0).toDouble();
    processingOptions.err[4] = settings.value("prcopt/err4", 1.0).toDouble();
    processingOptions.prn[0] = settings.value("prcopt/prn0", 1E-4).toDouble();
    processingOptions.prn[1] = settings.value("prcopt/prn1", 1E-3).toDouble();
    processingOptions.prn[2] = settings.value("prcopt/prn2", 1E-4).toDouble();
    processingOptions.prn[3] = settings.value("prcopt/prn3", 10.0).toDouble();
    processingOptions.prn[4] = settings.value("prcopt/prn4", 10.0).toDouble();
    processingOptions.sclkstab = settings.value("prcopt/sclkstab", 5E-12).toDouble();
    processingOptions.thresar[0] = settings.value("prcopt/thresar", 3.0).toDouble();
    processingOptions.elmaskar = settings.value("prcopt/elmaskar", 0.0).toDouble();
    processingOptions.elmaskhold = settings.value("prcopt/elmaskhold", 0.0).toDouble();
    processingOptions.thresslip = settings.value("prcopt/thresslip", 0.05).toDouble();
    processingOptions.maxtdiff = settings.value("prcopt/maxtdiff", 30.0).toDouble();
    processingOptions.maxinno[0] = settings.value("prcopt/maxinno1", 30.0).toDouble();
    processingOptions.maxinno[1] = settings.value("prcopt/maxinno2", 30.0).toDouble();
    processingOptions.syncsol = settings.value("prcopt/syncsol", 0).toInt();
    excludedSatellites = settings.value("prcopt/exsats", "").toString();
    processingOptions.navsys = settings.value("prcopt/navsys", SYS_GPS).toInt();
    processingOptions.posopt[0] = settings.value("prcopt/posopt1", 0).toInt();
    processingOptions.posopt[1] = settings.value("prcopt/posopt2", 0).toInt();
    processingOptions.posopt[2] = settings.value("prcopt/posopt3", 0).toInt();
    processingOptions.posopt[3] = settings.value("prcopt/posopt4", 0).toInt();
    processingOptions.posopt[4] = settings.value("prcopt/posopt5", 0).toInt();
    processingOptions.posopt[5] = settings.value("prcopt/posopt6", 0).toInt();
    processingOptions.maxaveep = settings.value("prcopt/maxaveep", 3600).toInt();
    processingOptions.initrst = settings.value("prcopt/initrst", 1).toInt();

    baselineEnabled = settings.value("prcopt/baselinec", 0).toInt();
    baseline[0] = settings.value("prcopt/baseline1", 0.0).toDouble();
    baseline[1] = settings.value("prcopt/baseline2", 0.0).toDouble();

    solutionOptions.posf = settings.value("solopt/posf", 0).toInt();
    solutionOptions.times = settings.value("solopt/times", 0).toInt();
    solutionOptions.timef = settings.value("solopt/timef", 1).toInt();
    solutionOptions.timeu = settings.value("solopt/timeu", 3).toInt();
    solutionOptions.degf = settings.value("solopt/degf", 0).toInt();
    solutionOptions.sep[0] = settings.value("solopt/sep", " ").toString()[0].toLatin1();
    solutionOptions.outhead = settings.value("solop/outhead", 0).toInt();
    solutionOptions.outopt = settings.value("solopt/outopt", 0).toInt();
    solutionOptions.outvel = settings.value("solopt/outvel", 0).toInt();
    processingOptions.outsingle = settings.value("prcopt/outsingle", 0).toInt();
    solutionOptions.maxsolstd = settings.value("solopt/maxsolstd", 0).toInt();
    solutionOptions.datum = settings.value("solopt/datum", 0).toInt();
    solutionOptions.height = settings.value("solopt/height", 0).toInt();
    solutionOptions.geoid = settings.value("solopt/geoid", 0).toInt();
    solutionOptions.nmeaintv[0] = settings.value("solopt/nmeaintv1", 0.0).toDouble();
    solutionOptions.nmeaintv[1] = settings.value("solopt/nmeaintv2", 0.0).toDouble();
    debugStatusLevel = settings.value("setting/debugstatus", 0).toInt();
    debugTraceLevel = settings.value("setting/debugtrace", 0).toInt();

    roverPositionTypeF = settings.value("setting/rovpostype", 0).toInt();
    referencePositionTypeF = settings.value("setting/refpostype", 0).toInt();
    roverAntennaPcvF = settings.value("setting/rovantpcv", 0).toInt();
    referenceAntennaPcvF = settings.value("setting/refantpcv", 0).toInt();
    roverAntennaF = settings.value("setting/rovant", "").toString();
    referenceAntennaF = settings.value("setting/refant", "").toString();
    satellitePcvFileF = settings.value("setting/satpcvfile", "").toString();
    antennaPcvFileF = settings.value("setting/antpcvfile", "").toString();
    stationPositionFileF = settings.value("setting/staposfile", "").toString();
    geoidDataFileF = settings.value("setting/geoiddatafile", "").toString();
    dcbFile = settings.value("setting/dcbfile", "").toString();
    eopFileF = settings.value("setting/eopfile", "").toString();
    localDirectory = settings.value("setting/localdirectory", "C:\\Temp").toString();

    serverCycle = settings.value("setting/svrcycle", 10).toInt();
    timeoutTime = settings.value("setting/timeouttime", 10000).toInt();
    reconnectionTime = settings.value("setting/recontime", 10000).toInt();
    nmeaCycle = settings.value("setting/nmeacycle", 5000).toInt();
    serverBufferSize = settings.value("setting/svrbuffsize", 32768).toInt();
    solutionBufferSize = settings.value("setting/solbuffsize", 1000).toInt();
    savedSolutions = settings.value("setting/savedsol", 100).toInt();
    NavSelect = settings.value("setting/navselect", 0).toInt();
    processingOptions.sbassatsel = settings.value("setting/sbassat", 0).toInt();
    dgpsCorrection = settings.value("setting/dgpscorr", 0).toInt();
    sbasCorrection = settings.value("setting/sbascorr", 0).toInt();

    NmeaReq = settings.value("setting/nmeareq", 0).toInt();
    inputTimeTag = settings.value("setting/intimetag", 0).toInt();
    inputTimeSpeed = settings.value("setting/intimespeed", "x1").toString();
    inputTimeStart = settings.value("setting/intimestart", "0").toString();
    inputTime64Bit = settings.value("setting/intime64bit", "0").toInt();
    outputTimeTag = settings.value("setting/outtimetag", 0).toInt();
    outputAppend = settings.value("setting/outappend", 0).toInt();
    outputSwapInterval = settings.value("setting/outswapinterval", "").toString();
    logTimeTag = settings.value("setting/logtimetag", 0).toInt();
    logAppend = settings.value("setting/logappend", 0).toInt();
    logSwapInterval = settings.value("setting/logswapinterval", "").toString();
    nmeaPosition[0] = settings.value("setting/nmeapos1", 0.0).toDouble();
    nmeaPosition[1] = settings.value("setting/nmeapos2", 0.0).toDouble();
    nmeaPosition[2] = settings.value("setting/nmeapos3", 0.0).toDouble();
    resetCommand = settings.value("setting/resetcmd", "").toString();
    maxBaseLine = settings.value("setting/maxbl", 10.0).toDouble();
    fileSwapMargin = settings.value("setting/fswapmargin", 30).toInt();

    timeSystem = settings.value("setting/timesys", 0).toInt();
    solutionType = settings.value("setting/soltype", 0).toInt();
    plotType[0] = settings.value("setting/plottype", 0).toInt();
    plotType[1] = settings.value("setting/plottype2", 0).toInt();
    plotType[2] = settings.value("setting/plottype3", 0).toInt();
    plotType[3] = settings.value("setting/plottype4", 0).toInt();
    panelMode = settings.value("setting/panelmode", 0).toInt();
    proxyAddress = settings.value("setting/proxyaddr", "").toString();
    monitorPort = settings.value("setting/moniport", DEFAULTPORT).toInt();
    panelStacking = settings.value("setting/panelstack", 0).toInt();
    trackType[0] = settings.value("setting/trktype1", 0).toInt();
    trackType[1] = settings.value("setting/trktype2", 0).toInt();
    trackType[2] = settings.value("setting/trktype3", 0).toInt();
    trackType[3] = settings.value("setting/trktype4", 0).toInt();
    trackScale[0] = settings.value("setting/trkscale1", 5).toInt();
    trackScale[1] = settings.value("setting/trkscale2", 5).toInt();
    trackScale[2] = settings.value("setting/trkscale3", 5).toInt();
    trackScale[3] = settings.value("setting/trkscale4", 5).toInt();
    frequencyType[0] = settings.value("setting/freqtype1", 0).toInt();
    frequencyType[1] = settings.value("setting/freqtype2", 0).toInt();
    frequencyType[2] = settings.value("setting/freqtype3", 0).toInt();
    frequencyType[3] = settings.value("setting/freqtype4", 0).toInt();
    baselineMode[0] = settings.value("setting/blmode1", 0).toInt();
    baselineMode[1] = settings.value("setting/blmode2", 0).toInt();
    baselineMode[2] = settings.value("setting/blmode3", 0).toInt();
    baselineMode[3] = settings.value("setting/blmode4", 0).toInt();
    markerName = settings.value("setting/markername", "").toString();
    markerComment = settings.value("setting/markercomment", "").toString();

    for (i = 0; i < 3; i++) {
        roverAntennaDelta[i] = settings.value(QString("setting/rovantdel_%1").arg(i), 0.0).toDouble();
        referenceAntennaDelta[i] = settings.value(QString("setting/refantdel_%1").arg(i), 0.0).toDouble();
        roverPosition   [i] = settings.value(QString("setting/rovpos_%1").arg(i), 0.0).toDouble();
        referencePosition   [i] = settings.value(QString("setting/refpos_%1").arg(i), 0.0).toDouble();
    }
    for (i = 0; i < 10; i++)
        history[i] = settings.value(QString("tcpopt/history%1").arg(i), "").toString();
    nMapPoint = settings.value("mapopt/nmappnt", 0).toInt();
    for (i = 0; i < nMapPoint; i++) {
        pointName[i] = settings.value(QString("mapopt/pntname%1").arg(i + 1), "").toString();
        QString pos = settings.value(QString("mapopt/pntpos%1").arg(i + 1), "0,0,0").toString();
        pointPosition[i][0] = pointPosition[i][1] = pointPosition[i][2] = 0.0;
        sscanf(qPrintable(pos), "%lf,%lf,%lf", pointPosition[i], pointPosition[i] + 1, pointPosition[i] + 2);
    }
    panelFont.setFamily(settings.value("setting/panelfontname", POSFONTNAME).toString());
    panelFont.setPointSize(settings.value("setting/panelfontsize", POSFONTSIZE).toInt());
    panelFontColor = QColor(static_cast<QRgb>(settings.value("setting/panelfontcolor", static_cast<int>(Qt::black)).toInt()));  // nowhere used
    if (settings.value("setting/panelfontbold", 0).toInt()) panelFont.setBold(true);
    if (settings.value("setting/panelfontitalic", 0).toInt()) panelFont.setItalic(true); ;
    positionFont.setFamily(settings.value("setting/posfontname", POSFONTNAME).toString());
    positionFont.setPointSize(settings.value("setting/posfontsize", POSFONTSIZE).toInt());
    positionFontColor = QColor(static_cast<QRgb>(settings.value("setting/posfontcolor", static_cast<int>(Qt::black)).toInt()));
    if (settings.value("setting/posfontbold", 0).toInt()) positionFont.setBold(true);
    if (settings.value("setting/posfontitalic", 0).toInt()) positionFont.setItalic(true); ;

    TextViewer::colorText = QColor(static_cast<QRgb>(settings.value("viewer/color1", static_cast<int>(Qt::black)).toInt()));
    TextViewer::colorBackground = QColor(static_cast<QRgb>(settings.value("viewer/color2", static_cast<int>(Qt::white)).toInt()));
    TextViewer::font.setFamily(settings.value("viewer/fontname", "Courier New").toString());
    TextViewer::font.setPointSize(settings.value("viewer/fontsize", 9).toInt());

    updatePanel();

    splitter->restoreState(settings.value("window/splitpos").toByteArray());

    resize(settings.value("window/width", 388).toInt(),
           settings.value("window/height", 284).toInt());
}
// save option to ini file --------------------------------------------------
void MainWindow::saveOptions()
{
    QSettings settings(iniFile, QSettings::IniFormat);
    int i, j, no, strno[] = { 0, 1, 6, 2, 3, 4, 5, 7 };

    trace(3, "SaveOpt\n");

    for (i = 0; i < 8; i++) {
        no = strno[i];
        settings.setValue(QString("stream/streamc%1").arg(no), streamEnabled[i]);
        settings.setValue(QString("stream/stream%1").arg(no), streamType[i]);
        settings.setValue(QString("stream/format%1").arg(no), inputFormat[i]);
        for (j = 0; j < 4; j++)
            settings.setValue(QString("stream/path_%1_%2").arg(no).arg(j), paths[i][j]);
    }
    for (i = 0; i < 3; i++)
        settings.setValue(QString("stream/rcvopt%1").arg(i + 1), receiverOptions[i]);

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
            commands[i][j].replace("\r\n", "@@");
            settings.setValue(QString("serial/cmd_%1_%2").arg(i).arg(j), commands[i][j]);
            settings.setValue(QString("serial/cmdena_%1_%2").arg(i).arg(j), commandEnabled[i][j]);
        }
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
            commandsTcp[i][j].replace("\r\n", "@@");
            settings.setValue(QString("tcpip/cmd_%1_%2").arg(i).arg(j), commandsTcp[i][j]);
            settings.setValue(QString("tcpip/cmdena_%1_%2").arg(i).arg(j), commandEnableTcp[i][j]);
        }
    settings.setValue("prcopt/mode", processingOptions.mode);
    settings.setValue("prcopt/nf", processingOptions.nf);
    settings.setValue("prcopt/elmin", processingOptions.elmin);
    settings.setValue("prcopt/snrmask_ena1", processingOptions.snrmask.ena[0]);
    settings.setValue("prcopt/snrmask_ena2", processingOptions.snrmask.ena[1]);
    for (i = 0; i < NFREQ; i++) for (j = 0; j < 9; j++)
            settings.setValue(QString("prcopt/snrmask_%1_%2").arg(i + 1).arg(j + 1),
                              processingOptions.snrmask.mask[i][j]);

    settings.setValue("prcopt/dynamics", processingOptions.dynamics);
    settings.setValue("prcopt/tidecorr", processingOptions.tidecorr);
    settings.setValue("prcopt/modear", processingOptions.modear);
    settings.setValue("prcopt/glomodear", processingOptions.glomodear);
    settings.setValue("prcopt/bdsmodear", processingOptions.bdsmodear);
    settings.setValue("prcopt/maxout", processingOptions.maxout);
    settings.setValue("prcopt/minlock", processingOptions.minlock);
    settings.setValue("prcopt/minfix", processingOptions.minfix);
    settings.setValue("prcopt/ionoopt", processingOptions.ionoopt);
    settings.setValue("prcopt/tropopt", processingOptions.tropopt);
    settings.setValue("prcopt/ephopt", processingOptions.sateph);
    settings.setValue("prcopt/niter", processingOptions.niter);
    settings.setValue("prcopt/eratio0", processingOptions.eratio[0]);
    settings.setValue("prcopt/eratio1", processingOptions.eratio[1]);
    settings.setValue("prcopt/err1", processingOptions.err[1]);
    settings.setValue("prcopt/err2", processingOptions.err[2]);
    settings.setValue("prcopt/err3", processingOptions.err[3]);
    settings.setValue("prcopt/err4", processingOptions.err[4]);
    settings.setValue("prcopt/prn0", processingOptions.prn[0]);
    settings.setValue("prcopt/prn1", processingOptions.prn[1]);
    settings.setValue("prcopt/prn2", processingOptions.prn[2]);
    settings.setValue("prcopt/prn3", processingOptions.prn[3]);
    settings.setValue("prcopt/prn4", processingOptions.prn[4]);
    settings.setValue("prcopt/sclkstab", processingOptions.sclkstab);
    settings.setValue("prcopt/thresar", processingOptions.thresar[0]);
    settings.setValue("prcopt/elmaskar", processingOptions.elmaskar);
    settings.setValue("prcopt/elmaskhold", processingOptions.elmaskhold);
    settings.setValue("prcopt/thresslip", processingOptions.thresslip);
    settings.setValue("prcopt/maxtdiff", processingOptions.maxtdiff);
    settings.setValue("prcopt/maxinno1", processingOptions.maxinno[0]);
    settings.setValue("prcopt/maxinno2", processingOptions.maxinno[1]);
    settings.setValue("prcopt/syncsol", processingOptions.syncsol);
    settings.setValue("prcopt/exsats", excludedSatellites);
    settings.setValue("prcopt/navsys", processingOptions.navsys);
    settings.setValue("prcopt/posopt1", processingOptions.posopt[0]);
    settings.setValue("prcopt/posopt2", processingOptions.posopt[1]);
    settings.setValue("prcopt/posopt3", processingOptions.posopt[2]);
    settings.setValue("prcopt/posopt4", processingOptions.posopt[3]);
    settings.setValue("prcopt/posopt5", processingOptions.posopt[4]);
    settings.setValue("prcopt/posopt6", processingOptions.posopt[5]);
    settings.setValue("prcopt/maxaveep", processingOptions.maxaveep);
    settings.setValue("prcopt/initrst", processingOptions.initrst);

    settings.setValue("prcopt/baselinec", baselineEnabled);
    settings.setValue("prcopt/baseline1", baseline[0]);
    settings.setValue("prcopt/baseline2", baseline[1]);

    settings.setValue("solopt/posf", solutionOptions.posf);
    settings.setValue("solopt/times", solutionOptions.times);
    settings.setValue("solopt/timef", solutionOptions.timef);
    settings.setValue("solopt/timeu", solutionOptions.timeu);
    settings.setValue("solopt/degf", solutionOptions.degf);
    settings.setValue("solopt/sep", QString(solutionOptions.sep[0]));
    settings.setValue("solopt/outhead", solutionOptions.outhead);
    settings.setValue("solopt/outopt", solutionOptions.outopt);
    settings.setValue("solopt/outvel", solutionOptions.outvel);
    settings.setValue("prcopt/outsingle", processingOptions.outsingle);
    settings.setValue("solopt/maxsolstd", solutionOptions.maxsolstd);
    settings.setValue("solopt/datum", solutionOptions.datum);
    settings.setValue("solopt/height", solutionOptions.height);
    settings.setValue("solopt/geoid", solutionOptions.geoid);
    settings.setValue("solopt/nmeaintv1", solutionOptions.nmeaintv[0]);
    settings.setValue("solopt/nmeaintv2", solutionOptions.nmeaintv[1]);
    settings.setValue("setting/debugstatus", debugStatusLevel);
    settings.setValue("setting/debugtrace", debugTraceLevel);

    settings.setValue("setting/rovpostype", roverPositionTypeF);
    settings.setValue("setting/refpostype", referencePositionTypeF);
    settings.setValue("setting/rovantpcv", roverAntennaPcvF);
    settings.setValue("setting/refantpcv", referenceAntennaPcvF);
    settings.setValue("setting/rovant", roverAntennaF);
    settings.setValue("setting/refant", referenceAntennaF);
    settings.setValue("setting/satpcvfile", satellitePcvFileF);
    settings.setValue("setting/antpcvfile", antennaPcvFileF);
    settings.setValue("setting/staposfile", stationPositionFileF);
    settings.setValue("setting/geoiddatafile", geoidDataFileF);
    settings.setValue("setting/dcbfile", dcbFile);
    settings.setValue("setting/eopfile", eopFileF);
    settings.setValue("setting/localdirectory", localDirectory);

    settings.setValue("setting/svrcycle", serverCycle);
    settings.setValue("setting/timeouttime", timeoutTime);
    settings.setValue("setting/recontime", reconnectionTime);
    settings.setValue("setting/nmeacycle", nmeaCycle);
    settings.setValue("setting/svrbuffsize", serverBufferSize);
    settings.setValue("setting/solbuffsize", solutionBufferSize);
    settings.setValue("setting/savedsol", savedSolutions);
    settings.setValue("setting/navselect", NavSelect);
    settings.setValue("setting/sbassat", processingOptions.sbassatsel);
    settings.setValue("setting/dgpscorr", dgpsCorrection);
    settings.setValue("setting/sbascorr", sbasCorrection);

    settings.setValue("setting/nmeareq", NmeaReq);
    settings.setValue("setting/intimetag", inputTimeTag);
    settings.setValue("setting/intimespeed", inputTimeSpeed);
    settings.setValue("setting/intimestart", inputTimeStart);
    settings.setValue("setting/intime64bit", inputTime64Bit);
    settings.setValue("setting/outtimetag", outputTimeTag);
    settings.setValue("setting/outappend", outputAppend);
    settings.setValue("setting/outswapinterval", outputSwapInterval);
    settings.setValue("setting/logtimetag", logTimeTag);
    settings.setValue("setting/logappend", logAppend);
    settings.setValue("setting/logswapinterval", logSwapInterval);
    settings.setValue("setting/nmeapos1", nmeaPosition[0]);
    settings.setValue("setting/nmeapos2", nmeaPosition[1]);
    settings.setValue("setting/nmeapos3", nmeaPosition[2]);
    settings.setValue("setting/resetcmd", resetCommand);
    settings.setValue("setting/maxbl", maxBaseLine);
    settings.setValue("setting/fswapmargin", fileSwapMargin);

    settings.setValue("setting/timesys", timeSystem);
    settings.setValue("setting/soltype", solutionType);
    settings.setValue("setting/plottype", plotType[0]);
    settings.setValue("setting/plottype2", plotType[1]);
    settings.setValue("setting/plottype3", plotType[2]);
    settings.setValue("setting/plottype4", plotType[3]);
    settings.setValue("setting/panelmode", panelMode);
    settings.setValue("setting/proxyaddr", proxyAddress);
    settings.setValue("setting/moniport", monitorPort);
    settings.setValue("setting/panelstack", panelStacking);
    settings.setValue("setting/trktype1", trackType[0]);
    settings.setValue("setting/trktype2", trackType[1]);
    settings.setValue("setting/trktype3", trackType[2]);
    settings.setValue("setting/trktype4", trackType[3]);
    settings.setValue("setting/trkscale1", trackScale[0]);
    settings.setValue("setting/trkscale2", trackScale[1]);
    settings.setValue("setting/trkscale3", trackScale[2]);
    settings.setValue("setting/trkscale4", trackScale[3]);
    settings.setValue("setting/freqtype1", frequencyType[0]);
    settings.setValue("setting/freqtype2", frequencyType[1]);
    settings.setValue("setting/freqtype3", frequencyType[2]);
    settings.setValue("setting/freqtype4", frequencyType[3]);
    settings.setValue("setting/blmode1", baselineMode[0]);
    settings.setValue("setting/blmode2", baselineMode[1]);
    settings.setValue("setting/blmode3", baselineMode[2]);
    settings.setValue("setting/blmode4", baselineMode[3]);
    settings.setValue("setting/markername", markerName);
    settings.setValue("setting/markercomment", markerComment);

    for (i = 0; i < 3; i++) {
        settings.setValue(QString("setting/rovantdel_%1").arg(i), roverAntennaDelta[i]);
        settings.setValue(QString("setting/refantdel_%1").arg(i), referenceAntennaDelta[i]);
        settings.setValue(QString("setting/rovpos_%1").arg(i), roverPosition[i]);
        settings.setValue(QString("setting/refpos_%1").arg(i), referencePosition[i]);
    }
    for (i = 0; i < 10; i++)
        settings.setValue(QString("tcpopt/history%1").arg(i), history[i]);
    settings.setValue("mapopt/nmappnt", nMapPoint);
    for (i = 0; i < nMapPoint; i++) {
        settings.setValue(QString("mapopt/pntname%1").arg(i + 1), pointName[i]);
        settings.setValue(QString("mapopt/pntpos%1").arg(i + 1),
                  QString("%1,%2,%3").arg(pointPosition[i][0], 0, 'f', 4).arg(pointPosition[i][1], 0, 'f', 4).arg(pointPosition[i][2], 0, 'f', 4));
    }
    settings.setValue("setting/panelfontname", panelFont.family());
    settings.setValue("setting/panelfontsize", panelFont.pointSize());
    settings.setValue("setting/panelfontcolor", static_cast<int>(panelFontColor.rgb()));
    settings.setValue("setting/panelfontbold", panelFont.bold());
    settings.setValue("setting/paneöfontitalic", panelFont.italic());

        settings.setValue("setting/posfontname", positionFont.family());
    settings.setValue("setting/posfontsize", positionFont.pointSize());
    settings.setValue("setting/posfontcolor", static_cast<int>(positionFontColor.rgb()));
    settings.setValue("setting/posfontbold", positionFont.bold());
    settings.setValue("setting/posfontitalic", positionFont.italic());

    settings.setValue("viewer/color1", static_cast<int>(TextViewer::colorText.rgb()));
    settings.setValue("viewer/color2", static_cast<int>(TextViewer::colorBackground.rgb()));
    settings.setValue("viewer/fontname", TextViewer::font.family());
    settings.setValue("viewer/fontsize", TextViewer::font.pointSize());

    settings.setValue("window/width", size().width());
    settings.setValue("window/height", size().height());

    settings.setValue("window/splitpos", splitter->saveState());
}
//---------------------------------------------------------------------------
void MainWindow::btnMarkClicked()
{
    QMarkDialog *markDialog = new QMarkDialog(this);

    markDialog->positionMode = rtksvr.rtk.opt.mode;
    markDialog->name = markerName;
    markDialog->comment = markerComment;

    if (markDialog->exec() != QDialog::Accepted) return;

    rtksvr.rtk.opt.mode = markDialog->positionMode;
    markerName = markDialog->name;
    markerComment = markDialog->comment;

    delete markDialog;
    
    updatePosition();
}
//---------------------------------------------------------------------------
