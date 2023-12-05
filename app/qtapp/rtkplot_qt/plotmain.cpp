//---------------------------------------------------------------------------
// rtkplot : visualization of solution and obs data ap
//
//          Copyright (C) 2007-2020 by T.TAKASU, All rights reserved.
//          ported to Qt by Jens Reimann
//
// options : rtkplot [-t title][-i file][-r][-p path][-x level][file ...]
//
//           -t title  window title
//           -i file   ini file path
//           -r        open file as obs and nav file
//           -p path   connect to path
//                       serial://port[:brate[:bsize[:parity[:stopb[:fctr]]]]]
//                       tcpsvr://:port
//                       tcpcli://addr[:port]
//                       ntrip://[user[:passwd]@]addr[:port][/mntpnt]
//                       file://path
//           -p1 path  connect port 1 to path
//           -p2 path  connect port 2 to path
//           -x level  debug trace level (0:off)
//           file      solution files or rinex obs/nav file
//
// version : $Revision: 1.1 $ $Date: 2008/07/17 22:15:27 $
// history : 2008/07/14  1.0 new
//           2009/11/27  1.1 rtklib 2.3.0
//           2010/07/18  1.2 rtklib 2.4.0
//           2010/06/10  1.3 rtklib 2.4.1
//           2010/06/19  1.4 rtklib 2.4.1 p1
//           2012/11/21  1.5 rtklib 2.4.2
//           2016/06/11  1.6 rtklib 2.4.3
//           2020/11/30  1.7 support NavIC/IRNSS
//                           delete functions for Google Earth View
//                           support Map API by Leaflet for Map View
//                           move Google Map API Key option to Map View
//                           modify order of ObsType selections
//                           improve slip detection performance by LG jump
//                           fix bug on MP computation for L3,L4,... freq.

//---------------------------------------------------------------------------
#include <clocale>

#include <QShowEvent>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QDebug>
#include <QToolBar>
#include <QScreen>
#include <QtGlobal>
#include <QFileInfo>

#include <QFileInfo>
#include <QCommandLineParser>
#include <QFileDialog>
#include <QClipboard>
#include <QSettings>
#include <QFont>
#include <QMimeData>

#define INHIBIT_RTK_LOCK_MACROS
#include "rtklib.h"
#include "plotmain.h"
#include "plotopt.h"
#include "refdlg.h"
#include "tspandlg.h"
#include "aboutdlg.h"
#include "conndlg.h"
#include "console.h"
#include "pntdlg.h"
#include "mapoptdlg.h"
#include "skydlg.h"
#include "freqdlg.h"
#include "mapview.h"
#include "viewer.h"
#include "vmapdlg.h"
#include "fileseldlg.h"
#include "helper.h"

#define YLIM_AGE    10.0        // ylimit of age of differential
#define YLIM_RATIO  20.0        // ylimit of ratio factor
#define MAXSHAPEFILE 16             // max number of shape files

static int RefreshTime = 100;    // update only every 100ms

extern "C" {
extern void settime(gtime_t) {}
extern void settspan(gtime_t, gtime_t) {}
}

// instance of Plot --------------------------------------------------------
Plot *plot = NULL;

// constructor --------------------------------------------------------------
Plot::Plot(QWidget *parent) : QMainWindow(parent)
{
    setupUi(this);

    plot = this;

    setWindowIcon(QIcon(":/icons/rtk2"));

    setlocale(LC_NUMERIC, "C"); // use point as decimal separator in formatted output

    gtime_t t0 = { 0, 0 };
    nav_t nav0;
    obs_t obs0 = { 0, 0, NULL };
    sta_t sta0;
    gis_t gis0 = {};
    solstatbuf_t solstat0 = { 0, 0, 0 };
    double ep[] = { 2000, 1, 1, 0, 0, 0 }, xl[2], yl[2];
    double xs[] = { -DEFTSPAN / 2, DEFTSPAN / 2 };

    memset(&nav0, 0, sizeof(nav_t));
    memset(&sta0, 0, sizeof(sta_t));

    QString file = QApplication::applicationFilePath();
    QFileInfo fi(file);
    iniFile = fi.absolutePath() + "/" + fi.baseName() + ".ini";

    toolBar->addWidget(Panel1);

    formWidth = formHeight = 0;
    Drag = 0; Xn = Yn = -1; nObservation = 0;
    indexObservation = NULL;
    azimuth = NULL; elevtion = NULL;
    week = flush = plotType = 0;
    animationCycle = 1;
    for (int i = 0; i < 2; i++) {
        initsolbuf(solutionData + i, 0, 0);
        solutionStat[i] = solstat0;
        solutionIndex[i] = 0;
    }
    observationIndex = 0;
    observation = obs0;
    navigation = nav0;
    station = sta0;
    gis = gis0;
    simObservation = 0;

    X0 = Y0 = Xc = Yc = Xs = Ys = Xcent = 0.0;
    mouseDownTick = 0;
    GEState = GEDataState[0] = GEDataState[1] = 0;
    GEHeading = 0.0;
    oEpoch = t0;
    for (int i = 0; i < 3; i++) oPosition[i] = oVelocity[i] = 0.0;
    azimuth = elevtion = NULL;
    for (int i = 0; i < NFREQ + NEXOBS; i++) multipath[i] = NULL;

    graphT = new Graph(lblDisplay);
    graphT->fit = 0;

    for (int i = 0; i < 3; i++) {
        graphG[i] = new Graph(lblDisplay);
        graphG[i]->xLabelPosition = 0;
        graphG[i]->getLimits(xl, yl);
        graphG[i]->setLimits(xs, yl);
    }
    graphR = new Graph(lblDisplay);
    for (int i = 0; i < 2; i++)
        graphE[i] = new Graph(lblDisplay);
    graphS = new Graph(lblDisplay);

    graphR->getLimits(xl, yl);
    graphR->setLimits(xs, yl);

    mapSize[0] = mapSize[1] = 0;
    mapScaleX = mapScaleY = 0.1;
    mapScaleEqual = 0;
    mapLatitude = mapLongitude = 0.0;
    pointType = 0;

    nWayPoint = 0;
    selectedWayPoint = -1;

    skySize[0] = skySize[1] = 0;
    skyCenter[0] = skyCenter[1] = 0;
    skyScale = skyScaleR = 240.0;
    skyFOV[0] = skyFOV[1] = skyFOV[2] = 0.0;
    skyElevationMask = 1;
    skyDestCorrection = skyRes = skyFlip = 0;

    for (int i = 0; i < 10; i++) skyDest[i] = 0.0;

    skyBinarize = 0;
    skyBinThres1 = 0.3;
    skyBinThres2 = 0.1;

    for (int i = 0; i < 3; i++) timeEnabled[i] = 0;
    timeLabel = autoScale = showStats = 0;
    showLabel = showGridLabel = 1;
    showArrow = showSlip = showHalfC = showError = showEphemeris = 0;
    plotStyle = markSize = origin = receiverPosition = 0;
    timeInterval = elevationMask = yRange = 0.0;
    maxDop = 30.0;
    maxMP = 10.0;
    timeStart = timeEnd = epoch2time(ep);
    console1 = new Console(this);
    console2 = new Console(this);
    lWRangeList->setParent(0);
    lWRangeList->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint);

    for (int i = 0; i < 361; i++) elevationMaskData[i] = 0.0;

    traceLevel = 0;
    connectState = openRaw = 0;
    rtConnectionType = 0;
    strinitcom();
    strinit(stream);
    strinit(stream + 1);

    cBFrequencyType->clear();
    cBFrequencyType->addItem("L1/LC");
    for (int i=0;i<NFREQ;i++) {
        cBFrequencyType->addItem(QString("L%1").arg(i+1));
    }
    cBFrequencyType->setCurrentIndex(0);

    tleData.n = tleData.nmax = 0;
    tleData.data = NULL;

    freqDialog = new FreqDialog(this);
    mapOptDialog = new MapOptDialog(this);
    mapView = new MapView(this);
    spanDialog = new SpanDialog(this);
    connectDialog = new ConnectDialog(this);
    skyImgDialog = new SkyImgDialog(this);
    plotOptDialog = new PlotOptDialog(this);
    aboutDialog = new AboutDialog(this);
    pntDialog = new PntDialog(this);
    fileSelDialog = new FileSelDialog(this);
    viewer = new TextViewer(this);

    btnConnect->setDefaultAction(MenuConnect);
    btnReload->setDefaultAction(MenuReload);
    btnClear->setDefaultAction(MenuClear);
    btnOptions->setDefaultAction(MenuOptions);
    btnMapView->setDefaultAction(MenuMapView);
    btnCenterOrigin->setDefaultAction(MenuCenterOri);
    btnFitHorizontal->setDefaultAction(MenuFitHoriz);
    btnFitVertical->setDefaultAction(MenuFitVert);
    btnShowTrack->setDefaultAction(MenuShowTrack);
    btnFixCenter->setDefaultAction(MenuFixCent);
    btnFixHorizontal->setDefaultAction(MenuFixHoriz);
    btnFixVertical->setDefaultAction(MenuFixVert);
    btnShowMap->setDefaultAction(MenuShowMap);
    btnShowGrid->setDefaultAction(MenuShowGrid);
    btnShowImage->setDefaultAction(MenuShowImg);
    btnShowSkyplot->setDefaultAction(MenuShowSkyplot);
    MenuShowSkyplot->setChecked(true);
    MenuShowGrid->setChecked(true);

    dirModel = new QFileSystemModel(this);
    dirModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    tVdirectorySelector = new QTreeView(this);
    Panel2->layout()->addWidget(tVdirectorySelector);
    tVdirectorySelector->setModel(dirModel);
    tVdirectorySelector->hideColumn(1); tVdirectorySelector->hideColumn(2); tVdirectorySelector->hideColumn(3); //only show names

    fileModel = new QFileSystemModel(this);
    fileModel->setFilter((fileModel->filter() & ~QDir::Dirs & ~QDir::AllDirs));
    fileModel->setNameFilterDisables(false);
    lVFileList->setModel(fileModel);


    connect(btnOn1, SIGNAL(clicked(bool)), this, SLOT(btnOn1Clicked()));
    connect(btnOn2, SIGNAL(clicked(bool)), this, SLOT(btnOn2Clicked()));
    connect(btnOn3, SIGNAL(clicked(bool)), this, SLOT(btnOn3Clicked()));
    connect(btnSolution1, SIGNAL(clicked(bool)), this, SLOT(btnSolution1Clicked()));//FIXME Double Click
    connect(btnSolution2, SIGNAL(clicked(bool)), this, SLOT(btnSolution2Clicked()));
    connect(btnSolution12, SIGNAL(clicked(bool)), this, SLOT(btnSolution12Clicked()));
    connect(btnRangeList, SIGNAL(clicked(bool)), this, SLOT(btnRangeListClicked()));
    connect(btnAnimate, SIGNAL(clicked(bool)), this, SLOT(btnAnimateClicked()));
    connect(MenuAbout, SIGNAL(triggered(bool)), this, SLOT(menuAboutClicked()));
    connect(MenuAnimStart, SIGNAL(triggered(bool)), this, SLOT(menuAnimationStartClicker()));
    connect(MenuAnimStop, SIGNAL(triggered(bool)), this, SLOT(menuAnimationStopClicker()));
    connect(MenuBrowse, SIGNAL(triggered(bool)), this, SLOT(menuBrowseClicked()));
    connect(MenuCenterOri, SIGNAL(triggered(bool)), this, SLOT(menuCenterOriginClicked()));
    connect(MenuClear, SIGNAL(triggered(bool)), this, SLOT(menuClearClicked()));
    connect(MenuConnect, SIGNAL(triggered(bool)), this, SLOT(menuConnectClicked()));
    connect(MenuDisconnect, SIGNAL(triggered(bool)), this, SLOT(menuDisconnectClicked()));
    connect(MenuFitHoriz, SIGNAL(triggered(bool)), this, SLOT(menuFitHorizontalClicked()));
    connect(MenuFitVert, SIGNAL(triggered(bool)), this, SLOT(menuFitVerticalClicked()));
    connect(MenuFixCent, SIGNAL(triggered(bool)), this, SLOT(menuFixCenterClicked()));
    connect(MenuFixHoriz, SIGNAL(triggered(bool)), this, SLOT(menuFixHorizontalClicked()));
    connect(MenuFixVert, SIGNAL(triggered(bool)), this, SLOT(menuFixVerticalClicked()));
    connect(MenuMapView, SIGNAL(triggered(bool)), this, SLOT(menuMapViewClicked()));
    connect(MenuMapImg, SIGNAL(triggered(bool)), this, SLOT(menuMapImageClicked()));
    connect(MenuMax, SIGNAL(triggered(bool)), this, SLOT(menuMaxClicked()));
    connect(MenuMonitor1, SIGNAL(triggered(bool)), this, SLOT(menuMonitor1Clicked()));
    connect(MenuMonitor2, SIGNAL(triggered(bool)), this, SLOT(menuMonitor2Clicked()));
    connect(MenuOpenElevMask, SIGNAL(triggered(bool)), this, SLOT(menuOpenElevationMaskClicked()));
    connect(MenuOpenMapImage, SIGNAL(triggered(bool)), this, SLOT(menuOpenMapImageClicked()));
    connect(MenuOpenShape, SIGNAL(triggered(bool)), this, SLOT(menuOpenShapeClicked()));
    connect(MenuOpenNav, SIGNAL(triggered(bool)), this, SLOT(menuOpenNavigatioClicked()));
    connect(MenuOpenObs, SIGNAL(triggered(bool)), this, SLOT(menuOpenObservationClicked()));
    connect(MenuOpenSkyImage, SIGNAL(triggered(bool)), this, SLOT(menuOpenSkyImageClicked()));
    connect(MenuOpenSol1, SIGNAL(triggered(bool)), this, SLOT(menuOpenSolution1Clicked()));
    connect(MenuOpenSol2, SIGNAL(triggered(bool)), this, SLOT(menuOpenSolution2Clicked()));
    connect(MenuOptions, SIGNAL(triggered(bool)), this, SLOT(menuOptionsClicked()));
    connect(MenuPlotMapView, SIGNAL(triggered(bool)), this, SLOT(menuPlotMapViewClicked()));
    connect(MenuPort, SIGNAL(triggered(bool)), this, SLOT(menuPortClicked()));
    connect(MenuShapeFile, SIGNAL(triggered(bool)), this, SLOT(MenuShapeFileClick()));
    connect(MenuQuit, SIGNAL(triggered(bool)), this, SLOT(menuQuitClicked()));
    connect(MenuReload, SIGNAL(triggered(bool)), this, SLOT(menuReloadClicked()));
    connect(MenuSaveDop, SIGNAL(triggered(bool)), this, SLOT(menuSaveDopClicked()));
    connect(MenuSaveElMask, SIGNAL(triggered(bool)), this, SLOT(menuSaveElevationMaskClicked()));
    connect(MenuSaveImage, SIGNAL(triggered(bool)), this, SLOT(menuSaveImageClicked()));
    connect(MenuSaveSnrMp, SIGNAL(triggered(bool)), this, SLOT(menuSaveSnrMpClicked()));
    connect(MenuShowMap, SIGNAL(triggered(bool)), this, SLOT(menuShowMapClicked()));
    connect(MenuShowImg, SIGNAL(triggered(bool)), this, SLOT(menuShowImageClicked()));
    connect(MenuShowGrid, SIGNAL(triggered(bool)), this, SLOT(menuShowGridClicked()));
    connect(MenuShowSkyplot, SIGNAL(triggered(bool)), this, SLOT(menuShowSkyplotClicked()));
    connect(MenuShowTrack, SIGNAL(triggered(bool)), this, SLOT(menuShowTrackClicked()));
    connect(MenuSkyImg, SIGNAL(triggered(bool)), this, SLOT(menuSkyImgClicked()));
    connect(MenuSrcObs, SIGNAL(triggered(bool)), this, SLOT(menuSrcObservationClicked()));
    connect(MenuSrcSol, SIGNAL(triggered(bool)), this, SLOT(menuSrcSolutionClicked()));
    connect(MenuStatusBar, SIGNAL(triggered(bool)), this, SLOT(menuStatusBarClicked()));
    connect(MenuTime, SIGNAL(triggered(bool)), this, SLOT(menuTimeClicked()));
    connect(MenuToolBar, SIGNAL(triggered(bool)), this, SLOT(menuToolBarClicked()));
    connect(MenuVisAna, SIGNAL(triggered(bool)), this, SLOT(menuVisibilityAnalysisClicked()));
    connect(MenuWaypnt, SIGNAL(triggered(bool)), this, SLOT(menuWaypointClicked()));
    connect(MenuMapLayer, SIGNAL(triggered(bool)), this, SLOT(menuMapLayerClicked()));
    connect(MenuOpenWaypoint, SIGNAL(triggered(bool)), this, SLOT(menuOpenWaypointClicked()));
    connect(MenuSaveWaypoint, SIGNAL(triggered(bool)), this, SLOT(menuSaveWaypointClicked()));
    connect(btnMessage2, SIGNAL(clicked(bool)), this, SLOT(btnMessage2Clicked()));
    connect(lWRangeList, SIGNAL(clicked(QModelIndex)), this, SLOT(rangeListClicked()));
    connect(&timer, SIGNAL(timeout()), this, SLOT(timerTimer()));
    connect(cBPlotTypeS, SIGNAL(currentIndexChanged(int)), this, SLOT(plotTypeSChanged()));
    connect(cBQFlag, SIGNAL(currentIndexChanged(int)), this, SLOT(qFlagChanged()));
    connect(sBTime, SIGNAL(valueChanged(int)), this, SLOT(timeScrollbarChanged()));
    connect(cBSatelliteList, SIGNAL(currentIndexChanged(int)), this, SLOT(satelliteListChanged()));
    connect(cBDopType, SIGNAL(currentIndexChanged(int)), this, SLOT(dopTypeChanged()));
    connect(cBObservationType, SIGNAL(currentIndexChanged(int)), this, SLOT(observationTypeChanged()));
    connect(cBObservationType2, SIGNAL(currentIndexChanged(int)), this, SLOT(observationTypeChanged()));
    connect(cBFrequencyType, SIGNAL(currentIndexChanged(int)), this, SLOT(observationTypeChanged()));
    connect(cBDriveSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(driveSelectChanged()));
    connect(tVdirectorySelector, SIGNAL(clicked(QModelIndex)), this, SLOT(directorySelectionChanged(QModelIndex)));
    connect(tVdirectorySelector, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(directorySelectionSelected(QModelIndex)));
    connect(btnDirectorySelect, SIGNAL(clicked(bool)), this, SLOT(btnDirectorySelectorClicked()));
    connect(lVFileList, SIGNAL(clicked(QModelIndex)), this, SLOT(fileListClicked(QModelIndex)));
    connect(cBFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(filterClicked()));


    bool state = false;
#ifdef QWEBKIT
    state = true;
#endif
#ifdef QWEBENGINE
    state = true;
#endif
    MenuMapView->setEnabled(state);
    MenuPlotMapView->setEnabled(state);

    lblDisplay->setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);

    loadOptions();

    updateTimer.start();
    qApp->installEventFilter(this);
}
// destructor ---------------------------------------------------------------
Plot::~Plot()
{
    delete [] indexObservation;
    delete [] azimuth;
    delete [] elevtion;

    for (int i = 0; i < NFREQ + NEXOBS; i++) delete multipath[i];

    delete graphT;
    delete graphR;
    delete graphS;
    for (int i = 0; i < 2; i++)
        delete graphE[i];
    for (int i = 0; i < 3; i++)
        delete graphG[i];
    delete lWRangeList;

    delete tVdirectorySelector;
}
// callback on all events ----------------------------------------------------
bool Plot::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == lblDisplay) && (event->type() == QEvent::MouseMove))
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (childAt(mouseEvent->pos()) == lblDisplay)
            mouseMove(mouseEvent);
    }
    if ((obj == lblDisplay) && (event->type() == QEvent::Resize))
    {
        updateSize();
        refresh();
    }

    return false;
}
// callback on form-show ----------------------------------------------------
void Plot::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) {
        refresh();
        return;
    }
    QString path1 = "", path2 = "";
    char str_path[256];

    trace(3, "FormShow\n");

    QCommandLineParser parser;
    parser.setApplicationDescription("RTK plot");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption rawOption(QStringList() << "r",
                     QCoreApplication::translate("main", "open raw"));
    parser.addOption(rawOption);

    QCommandLineOption titleOption(QStringList() << "t" << "title",
                       QCoreApplication::translate("main", "use window tile <title>."),
                       QCoreApplication::translate("main", "title"));
    parser.addOption(titleOption);

    QCommandLineOption pathOption(QStringList() << "p" << "path",
                      QCoreApplication::translate("main", "open <path>."),
                      QCoreApplication::translate("main", "path"));
    parser.addOption(pathOption);

    QCommandLineOption path1Option(QStringList() << "1",
                       QCoreApplication::translate("main", "open <path> as solution 1."),
                       QCoreApplication::translate("main", "path"));
    parser.addOption(path1Option);

    QCommandLineOption path2Option(QStringList() << "2",
                       QCoreApplication::translate("main", "open <path> as solution 2."),
                       QCoreApplication::translate("main", "path"));
    parser.addOption(path2Option);

    QCommandLineOption traceOption(QStringList() << "x" << "tracelevel",
                       QCoreApplication::translate("main", "set trace lavel to <tracelavel>."),
                       QCoreApplication::translate("main", "tracelevel"));
    parser.addOption(traceOption);

    parser.addPositionalArgument("file", QCoreApplication::translate("main", "Read files."));

    parser.process(*QApplication::instance());

    if (parser.isSet(rawOption))
        openRaw = 1;
    if (parser.isSet(titleOption))
        title = parser.value(titleOption);
    if (parser.isSet(pathOption))
        path1 = parser.value(pathOption);
    if (parser.isSet(path1Option))
        path1 = parser.value(path1Option);
    if (parser.isSet(path2Option))
        path2 = parser.value(path2Option);
    if (parser.isSet(traceOption))
        traceLevel = parser.value(traceOption).toInt();

    const QStringList args = parser.positionalArguments();
    QString file;
    foreach(file, args)
    openFiles.append(file);

    if (traceLevel>0) {
        traceopen(TRACEFILE);
        tracelevel(traceLevel);
    }
    loadOptions();

    updateType(plotType >= PLOT_OBS ? PLOT_TRK : plotType);

    updateColor();
    updateSatelliteMask();
    updateOrigin();
    updateSize();

    if (path1 != "" || path2 != "") {
        connectPath(path1, 0);
        connectPath(path2, 1);
        connectStream();
    } else if (openFiles.count() <= 0) {
        setWindowTitle(title != "" ? title : QString(tr("%1 ver. %2 %3")).arg(PRGNAME).arg(VER_RTKLIB).arg(PATCH_LEVEL));
    }
    if (shapeFile!="") {
        QStringList files;
        char *paths[MAXSHAPEFILE];
        for (int i=0;i<MAXSHAPEFILE;i++) {
            paths[i]=new char [1024];
        }
        int n=expath(qPrintable(shapeFile),paths,MAXSHAPEFILE);

        for (int i=0;i<n;i++) {
            files.append(paths[i]);
        }
        readShapeFile(files);

        for (int i=0;i<MAXSHAPEFILE;i++) {
            delete [] paths[i];
        }
    }
    if (tleFile != "")
        tle_read(qPrintable(tleFile), &tleData);
    if (tleSatelliteFile != "")
        tle_name_read(qPrintable(tleSatelliteFile), &tleData);

    QFileInfoList drives = QDir::drives();
    if (drives.size() > 1 && drives.at(0).filePath() != "/") {
        Panel1->setVisible(true);
        cBDriveSelect->clear();

        foreach(const QFileInfo &drive, drives) {
            cBDriveSelect->addItem(drive.filePath());
        }
    } else {
        Panel1->setVisible(false); // do not show drive selection on unix
    }
    if (directory == "") directory = drives.at(0).filePath();

    cBDriveSelect->setCurrentText(directory.mid(0, directory.indexOf(":") + 2));
    dirModel->setRootPath(directory);
    tVdirectorySelector->setVisible(false);
    lblDirectorySelected->setText(directory);
    fileModel->setRootPath(directory);
    lVFileList->setRootIndex(fileModel->index(directory));
    filterClicked();

    if (MenuBrowse->isChecked()) {
        panelBrowse->setVisible(true);
    }
    else {
        panelBrowse->setVisible(false);
    }

    strinit(&streamTimeSync);
    nStreamBuffer=0;

    if (timeSyncOut) {
        sprintf(str_path,":%d",timeSyncPort);
        stropen(&streamTimeSync,STR_TCPSVR,STR_MODE_RW,str_path);
    }

    timer.start(refreshCycle);
    updatePlot();
    updateEnable();

    if (openFiles.count() > 0) {
        if (checkObservation(openFiles.at(0)) || openRaw) readObservation(openFiles);
        else readSolution(openFiles, 0);
    }
}
// callback on form-close ---------------------------------------------------
void Plot::closeEvent(QCloseEvent *)
{
    trace(3, "FormClose\n");

    lWRangeList->setVisible(false);

    saveOption();

    if (traceLevel > 0) traceclose();
}

// callback for painting   --------------------------------------------------
void Plot::paintEvent(QPaintEvent *)
{
    updateDisplay();
}
// callback on form-resize --------------------------------------------------
void Plot::resizeEvent(QResizeEvent *)
{
    trace(3, "FormResize\n");

    // suppress repeated resize callback
    if (formWidth == width() && formHeight == height()) return;

    updateSize();
    refresh();

    formWidth = width();
    formHeight = height();
}
// callback on drag-and-drop files ------------------------------------------
void Plot::dragEnterEvent(QDragEnterEvent *event)
{
    trace(3, "dragEnterEvent\n");

    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

// callback on drag-and-drop files ------------------------------------------
void Plot::dropEvent(QDropEvent *event)
{
    QStringList files;
    int n;

    trace(3, "dropEvent\n");

    if (connectState || !event->mimeData()->hasUrls())
        return;
    foreach(QUrl url, event->mimeData()->urls()) {
        files.append(QDir::toNativeSeparators(url.toString()));
    }

    QString file=files.at(0);

    if (files.size() == 1 && (n = files.at(0).lastIndexOf('.')) != -1) {
        QString ext = files.at(0).mid(n).toLower();
        if ((ext == "jpg") || (ext == "jpeg")) {
            if (plotType == PLOT_TRK)
                readMapData(file);
            else if (plotType == PLOT_SKY || plotType == PLOT_MPS)
                readSkyData(file);
        }
        ;
    } else if (checkObservation(files.at(0))) {
        readObservation(files);
    } else if (!btnSolution1->isChecked() && btnSolution2->isChecked()) {
        readSolution(files, 1);
    } else {
        readSolution(files, 0);
    }
}
// callback on menu-open-solution-1 -----------------------------------------
void Plot::menuOpenSolution1Clicked()
{
    trace(3, "MenuOpenSol1Click\n");

    readSolution(QStringList(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open Solution 1"), QString(), tr("Solution File (*.pos *.stat *.nmea *.txt *.ubx);;All (*.*)")))), 0);
}
// callback on menu-open-solution-2 -----------------------------------------
void Plot::menuOpenSolution2Clicked()
{
    trace(3, "MenuOpenSol2Click\n");

    readSolution(QStringList(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open Solution 2"), QString(), tr("Solution File (*.pos *.stat *.nmea *.txt *.ubx);;All (*.*)")))), 1);
}
// callback on menu-open-map-image ------------------------------------------
void Plot::menuOpenMapImageClicked()
{
    trace(3, "MenuOpenMapImage\n");

    readMapData(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open Map Image"), mapImageFile, tr("JPEG File (*.jpg *.jpeg);;All (*.*)"))));
}
// callback on menu-open-track-points ---------------------------------------
void Plot::menuOpenShapeClicked()
{
    trace(3, "MenuOpenShapePath\n");

    QStringList files = QFileDialog::getOpenFileNames(this, tr("Open Shape File"), QString(), tr("Shape File (*.shp);;All (*.*)"));
    for (int i = 0; i < files.size(); i++)
        files[i] = QDir::toNativeSeparators(files.at(i));

    readShapeFile(files);
}
// callback on menu-open-sky-image ------------------------------------------
void Plot::menuOpenSkyImageClicked()
{
    trace(3, "MenuOpenSkyImage\n");

    readSkyData(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open Sky Image"), skyImageFile, tr("JPEG File (*.jpg *.jpeg);;All (*.*)"))));
}
// callback on menu-oepn-waypoint -------------------------------------------
void Plot::menuOpenWaypointClicked()
{
    trace(3, "MenuOpenWaypointClick\n");

    readWaypoint(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open Waypoint"), skyImageFile, tr("Waypoint File (*.gpx, *.pos);;All (*.*)"))));
}
// callback on menu-open-obs-data -------------------------------------------
void Plot::menuOpenObservationClicked()
{
    trace(3, "MenuOpenObsClick\n");
    readObservation(QFileDialog::getOpenFileNames(this, tr("Open Obs/Nav Data"), QString(), tr("RINEX OBS (*.obs *.*o *.*d *.O.rnx *.*o.gz *.*o.Z *.d.gz *.d.Z);;All (*.*)")));
}
// callback on menu-open-nav-data -------------------------------------------
void Plot::menuOpenNavigatioClicked()
{
    trace(3, "MenuOpenNavClick\n");

    readNavigation(QFileDialog::getOpenFileNames(this, tr("Open Raw Obs/Nav Messages"), QString(), tr("RINEX NAV (*.nav *.gnav *.hnav *.qnav *.*n *.*g *.*h *.*q *.*p *N.rnx);;All (*.*)")));
}
// callback on menu-open-elev-mask ------------------------------------------
void Plot::menuOpenElevationMaskClicked()
{
    trace(3, "MenuOpenElevMaskClick\n");

    readElevationMaskData(QDir::toNativeSeparators(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Opene Elevation Mask"), QString(), tr("Text File (*.txt);;All (*.*)")))));
}
// callback on menu-vis-analysis --------------------------------------------
void Plot::menuVisibilityAnalysisClicked()
{
    if (receiverPosition != 1) { // lat/lon/height
        showMessage(tr("specify Receiver Position as Lat/Lon/Hgt"));
        return;
    }
    if (spanDialog->TimeStart.time == 0) {
        int week;
        double tow = time2gpst(utc2gpst(timeget()), &week);
        spanDialog->TimeStart = gpst2time(week, floor(tow / 3600.0) * 3600.0);
        spanDialog->TimeEnd = timeadd(spanDialog->TimeStart, 86400.0);
        spanDialog->TimeInt = 30.0;
    }
    spanDialog->TimeEna[0] = spanDialog->TimeEna[1] = spanDialog->TimeEna[2] = 1;
    spanDialog->TimeVal[0] = spanDialog->TimeVal[1] = spanDialog->TimeVal[2] = 2;

    spanDialog->exec();

    if (spanDialog->result() == QDialog::Accepted) {
        timeStart = spanDialog->TimeStart;
        timeEnd = spanDialog->TimeEnd;
        timeInterval = spanDialog->TimeInt;
        generateVisibilityData();
    }
    spanDialog->TimeVal[0] = spanDialog->TimeVal[1] = spanDialog->TimeVal[2] = 1;
}
// callback on menu-save image ----------------------------------------------
void Plot::menuSaveImageClicked()
{
    buffer.save(QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Save Image"), QString(), tr("JPEG  (*.jpg);;Windows Bitmap (*.bmp)"))));
}
// callback on menu-save-waypoint -------------------------------------------
void Plot::menuSaveWaypointClicked()
{
    trace(3, "MenuSaveWaypointClick\n");

    saveWaypoint(QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Save Waypoint"), QString(), tr("GPX File (*.gpx, *.pos);;All (*.*)"))));
}
// callback on menu-save-# of sats/dop --------------------------------------
void Plot::menuSaveDopClicked()
{
    trace(3, "MenuSaveDopClick\n");

    saveDop(QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Save Data"), QString(), tr("All (*.*);;Text File (*.txt)"))));
}
// callback on menu-save-snr,azel -------------------------------------------
void Plot::menuSaveSnrMpClicked()
{
    trace(3, "MenuSaveSnrMpClick\n");

    saveSnrMp(QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Save Data"), QString(), tr("All (*.*);;Text File (*.txt)"))));
}
// callback on menu-save-elmask ---------------------------------------------
void Plot::menuSaveElevationMaskClicked()
{
    trace(3, "MenuSaveElMaskClick\n");

    saveElevationMask(QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Save Data"), QString(), tr("All (*.*);;Text File (*.txt)"))));
}
// callback on menu-connect -------------------------------------------------
void Plot::menuConnectClicked()
{
    trace(3, "MenuConnectClick\n");

    connectStream();
}
// callback on menu-disconnect ----------------------------------------------
void Plot::menuDisconnectClicked()
{
    trace(3, "MenuDisconnectClick\n");

    disconnectStream();
}
// callback on menu-connection-settings -------------------------------------
void Plot::menuPortClicked()
{
    int i;

    trace(3, "MenuPortClick\n");

    connectDialog->stream1 = rtStream[0];
    connectDialog->stream2 = rtStream[1];
    connectDialog->format1 = rtFormat[0];
    connectDialog->format2 = rtFormat[1];
    connectDialog->timeFormat = rtTimeFormat;
    connectDialog->degFormat = rtDegFormat;
    connectDialog->fieldSeparator = rtFieldSeperator;
    connectDialog->timeoutTime = rtTimeoutTime;
    connectDialog->reconnectTime = rtReconnectTime;
    for (i = 0; i < 3; i++) {
        connectDialog->paths1[i] = streamPaths[0][i];
        connectDialog->paths2[i] = streamPaths[1][i];
    }
    for (i = 0; i < 2; i++) {
        connectDialog->commands1  [i] = streamCommands[0][i];
        connectDialog->commands2  [i] = streamCommands[1][i];
        connectDialog->commandEnable1[i] = streamCommandEnabled[0][i];
        connectDialog->commandEnable2[i] = streamCommandEnabled[1][i];
    }
    for (i = 0; i < 10; i++) connectDialog->TcpHistory [i] = streamHistory [i];

    connectDialog->exec();

    if (connectDialog->result() != QDialog::Accepted) return;

    rtStream[0] = connectDialog->stream1;
    rtStream[1] = connectDialog->stream2;
    rtFormat[0] = connectDialog->format1;
    rtFormat[1] = connectDialog->format2;
    rtTimeFormat = connectDialog->timeFormat;
    rtDegFormat = connectDialog->degFormat;
    rtFieldSeperator = connectDialog->fieldSeparator;
    rtTimeoutTime = connectDialog->timeoutTime;
    rtReconnectTime = connectDialog->reconnectTime;
    for (i = 0; i < 3; i++) {
        streamPaths[0][i] = connectDialog->paths1[i];
        streamPaths[1][i] = connectDialog->paths2[i];
    }
    for (i = 0; i < 2; i++) {
        streamCommands  [0][i] = connectDialog->commands1  [i];
        streamCommands  [1][i] = connectDialog->commands2  [i];
        streamCommandEnabled[0][i] = connectDialog->commandEnable1[i];
        streamCommandEnabled[1][i] = connectDialog->commandEnable2[i];
    }
    for (i = 0; i < 10; i++) streamHistory [i] = connectDialog->TcpHistory [i];
}
// callback on menu-reload --------------------------------------------------
void Plot::menuReloadClicked()
{
    trace(3, "MenuReloadClick\n");

    reload();
}
// callback on menu-clear ---------------------------------------------------
void Plot::menuClearClicked()
{
    trace(3, "MenuClearClick\n");

    clear();
}
// callback on menu-exit-----------------------------------------------------
void Plot::menuQuitClicked()
{
    trace(3, "MenuQuitClick\n");

    close();
}
// callback on menu-time-span/interval --------------------------------------
void Plot::menuTimeClicked()
{
    sol_t *sols, *sole;
    int i;

    trace(3, "MenuTimeClick\n");

    if (!timeEnabled[0]) {
        if (observation.n > 0) {
            timeStart = observation.data[0].time;
        } else if (btnSolution2->isChecked() && solutionData[1].n > 0) {
            sols = getsol(solutionData + 1, 0);
            timeStart = sols->time;
        } else if (solutionData[0].n > 0) {
            sols = getsol(solutionData, 0);
            timeStart = sols->time;
        }
    }
    if (!timeEnabled[1]) {
        if (observation.n > 0) {
            timeEnd = observation.data[observation.n - 1].time;
        } else if (btnSolution2->isChecked() && solutionData[1].n > 0) {
            sole = getsol(solutionData + 1, solutionData[1].n - 1);
            timeEnd = sole->time;
        } else if (solutionData[0].n > 0) {
            sole = getsol(solutionData, solutionData[0].n - 1);
            timeEnd = sole->time;
        }
    }
    for (i = 0; i < 3; i++)
        spanDialog->TimeEna[i] = timeEnabled[i];

    spanDialog->TimeStart = timeStart;
    spanDialog->TimeEnd = timeEnd;
    spanDialog->TimeInt = timeInterval;
    spanDialog->TimeVal[0] = !connectState;
    spanDialog->TimeVal[1] = !connectState;

    spanDialog->exec();

    if (spanDialog->result() != QDialog::Accepted) return;

    if (timeEnabled[0] != spanDialog->TimeEna[0] ||
        timeEnabled[1] != spanDialog->TimeEna[1] ||
        timeEnabled[2] != spanDialog->TimeEna[2] ||
        timediff(timeStart, spanDialog->TimeStart) != 0.0 ||
        timediff(timeEnd, spanDialog->TimeEnd) != 0.0 ||
        !qFuzzyCompare(timeInterval, spanDialog->TimeInt)) {
        for (i = 0; i < 3; i++) timeEnabled[i] = spanDialog->TimeEna[i];

        timeStart = spanDialog->TimeStart;
        timeEnd = spanDialog->TimeEnd;
        timeInterval = spanDialog->TimeInt;

        reload();
    }
}
// callback on menu-map-image -----------------------------------------------
void Plot::menuMapImageClicked()
{
    trace(3, "MenuMapImgClick\n");

    mapOptDialog->show();
}
// callback on menu-sky image -----------------------------------------------
void Plot::menuSkyImgClicked()
{
    trace(3, "MenuSkyImgClick\n");

    skyImgDialog->show();
}
// callback on menu-vec map -------------------------------------------------
void Plot::menuMapLayerClicked()
{
    trace(3, "MenuMapLayerClick\n");

    vecMapDialog = new VecMapDialog(this);

    vecMapDialog->exec();

    delete vecMapDialog;

}
// callback on menu-solution-source -----------------------------------------
void Plot::menuSrcSolutionClicked()
{
    int sel = !btnSolution1->isChecked() && btnSolution2->isChecked();

    trace(3, "MenuSrcSolClick\n");

    if (solutionFiles[sel].count() <= 0) return;

    viewer->setWindowTitle(solutionFiles[sel].at(0));
    viewer->option = 0;
    viewer->show();
    viewer->read(solutionFiles[sel].at(0));
}
// callback on menu-obs-data-source -----------------------------------------
void Plot::menuSrcObservationClicked()
{
    TextViewer *viewer;
    char file[1024], tmpfile[1024];
    int cstat;

    trace(3, "MenuSrcObsClick\n");

    if (observationFiles.count() <= 0) return;

    strcpy(file, qPrintable(observationFiles.at(0)));
    cstat = rtk_uncompress(file, tmpfile);

    viewer = new TextViewer(this);
    viewer->setWindowTitle(observationFiles.at(0));
    viewer->option = 0;
    viewer->show();
    viewer->read(!cstat ? file : tmpfile);
    if (cstat) remove(tmpfile);
}
// callback on menu-copy-to-clipboard ---------------------------------------
void Plot::menuCopyClicked()
{
    trace(3, "MenuCopyClick\n");

    QClipboard *clipboard = QApplication::clipboard();

    clipboard->setPixmap(buffer);
}
// callback on menu-options -------------------------------------------------
void Plot::menuOptionsClicked()
{
    QString tlefile = tleFile, tlesatfile = tleSatelliteFile;
    double oopos[3];
    char str_path[256];
    int timesyncout=timeSyncOut, rcvpos=receiverPosition;

    trace(3, "MenuOptionsClick\n");

    matcpy(oopos,ooPosition,3,1);

    plotOptDialog->move(pos().x() + width() / 2 - plotOptDialog->width() / 2,
                pos().y() + height() / 2 - plotOptDialog->height() / 2);
    plotOptDialog->plot = this;

    plotOptDialog->exec();

    if (plotOptDialog->result() != QDialog::Accepted) return;

    saveOption();

    for (int i = 0; i < 3; i++) oopos[i] -= ooPosition[i];

    if (tleFile != tlefile) {
        free(tleData.data);
        tleData.data = NULL;
        tleData.n = tleData.nmax = 0;
        tle_read(qPrintable(tleFile), &tleData);
    }
    if (tleFile != tlefile || tleSatelliteFile != tlesatfile)
        tle_name_read(qPrintable(tleSatelliteFile), &tleData);
    if (rcvpos != receiverPosition || norm(oopos, 3) > 1E-3 || tleFile != tlefile) {
        if (simObservation) generateVisibilityData(); else updateObservation(nObservation);
    }
    updateColor();
    updateSize();
    updateOrigin();
    updateInfo();
    updateSatelliteMask();
    updateSatelliteList();

    refresh();
    timer.start(refreshCycle);

    for (int i = 0; i < lWRangeList->count(); i++) {
        QString str=lWRangeList->item(i)->text();
        double range;
        QString unit;

        QStringList tokens = str.split(' ');
        if (tokens.length()==2) {
            bool ok;
            range = tokens.at(0).toInt(&ok);
            unit = tokens.at(1);
            if (ok) {
                if (unit == "cm") range*=0.01;
                else if (unit == "km") range*=1000.0;
                if (range==yRange) {
                    lWRangeList->item(i)->setSelected(true);
                    break;
                }

            }
        }
    }
    if (!timesyncout && timeSyncOut) {
        sprintf(str_path,":%d",timeSyncPort);
        stropen(&streamTimeSync,STR_TCPSVR,STR_MODE_RW,str_path);
    }
    else if (timesyncout && !timeSyncOut) {
        strclose(&streamTimeSync);
    }
}
// callback on menu-show-tool-bar -------------------------------------------
void Plot::menuToolBarClicked()
{
    trace(3, "MenuToolBarClick\n");

    toolBar->setVisible(MenuToolBar->isChecked());

    updateSize();
    refresh();
}
// callback on menu-show-status-bar -----------------------------------------
void Plot::menuStatusBarClicked()
{
    trace(3, "MenuStatusBarClick\n");

    statusbar->setVisible(MenuStatusBar->isChecked());
    updateSize();
    refresh();
}
// callback on menu-show-browse-panel ---------------------------------------
void Plot::menuBrowseClicked()
{
    trace(3,"MenuBrowseClick\n");

    if (MenuBrowse->isChecked()) {
        panelBrowse->setVisible(true);
    }
    else {
        panelBrowse->setVisible(false);
    }

    lblDisplay->updateGeometry();

    updateSize();
    refresh();
}
// callback on menu-waypoints -----------------------------------------------
void Plot::menuWaypointClicked()
{
    trace(3, "MenuWaypointClick\n");

    pntDialog->show();
}
// callback on menu-input-monitor-1 -----------------------------------------
void Plot::menuMonitor1Clicked()
{
    trace(3, "MenuMonitor1Click\n");

    console1->setWindowTitle(tr("Monitor RT Input 1"));
    console1->show();
}
// callback on menu-input-monitor-2 -----------------------------------------
void Plot::menuMonitor2Clicked()
{
    trace(3, "MenuMonitor2Click\n");

    console2->setWindowTitle(tr("Monitor RT Input 2"));
    console2->show();
}
// callback on menu-map-view ---------------------------------------
void Plot::menuMapViewClicked()
{
    trace(3, "MenuGEClick\n");

    mapView->setWindowTitle(
        QString(tr("%1 ver.%2 %3: Google Earth View")).arg(PRGNAME).arg(VER_RTKLIB).arg(PATCH_LEVEL));
    mapView->show();
}
// callback on menu-center-origin -------------------------------------------
void Plot::menuCenterOriginClicked()
{
    trace(3, "MenuCenterOriClick\n");

    setRange(0, yRange);
    refresh();
}
// callback on menu-fit-horizontal ------------------------------------------
void Plot::menuFitHorizontalClicked()
{
    trace(3, "MenuFitHorizClick\n");

    if (plotType == PLOT_TRK) fitRange(0); else fitTime();
    refresh();
}
// callback on menu-fit-vertical --------------------------------------------
void Plot::menuFitVerticalClicked()
{
    trace(3, "MenuFitVertClick\n");

    fitRange(0);
    refresh();
}
// callback on menu-show-skyplot --------------------------------------------
void Plot::menuShowSkyplotClicked()
{
    trace(3, "MenuShowSkyplotClick\n");

    updatePlot();
    updateEnable();
}
// callback on menu-show-map-image ------------------------------------------
void Plot::menuShowImageClicked()
{
    trace(3, "MenuShowImgClick\n");

    updatePlot();
    updateEnable();
}
// callback on menu-show-grid -----------------------------------------------
void Plot::menuShowGridClicked()
{
    trace(3,"MenuShowGrid\n");

    updatePlot();
    updateEnable();
    refresh();
}
// callback on menu-show-track-points ---------------------------------------
void Plot::menuShowTrackClicked()
{
    trace(3, "MenuShowTrackClick\n");

    if (!MenuShowTrack->isChecked()) {
        MenuFixHoriz->setChecked(false);
        MenuFixVert->setChecked(false);
    }
    updatePlot();
    updateEnable();
}
// callback on menu-fix-center ----------------------------------------------
void Plot::menuFixCenterClicked()
{
    trace(3, "MenuFixCentClick\n");

    updatePlot();
    updateEnable();
}
// callback on menu-fix-horizontal ------------------------------------------
void Plot::menuFixHorizontalClicked()
{
    trace(3, "MenuFixHorizClick\n");

    Xcent = 0.0;
    updatePlot();
    updateEnable();
}
// callback on menu-fix-vertical --------------------------------------------
void Plot::menuFixVerticalClicked()
{
    trace(3, "MenuFixVertClick\n");

    updatePlot();
    updateEnable();
}
// callback on menu-show-map -------------------------------------------------
void Plot::menuShowMapClicked()
{
    trace(3, "MenuShowMapClick\n");

    updatePlot();
    updateEnable();
}
// callback on menu-windows-maximize ----------------------------------------
void Plot::menuMaxClicked()
{
    QScreen *scr = QApplication::screens().at(0);
    QRect rect = scr->availableGeometry();
    QSize thisDecoration = this->frameSize() - this->size();

    this->move(rect.x(), rect.y());
    this->resize(rect.width() - thisDecoration.width(), rect.height() - thisDecoration.height());

    mapView->hide();
}
// callback on menu-windows-mapview -----------------------------------------
void Plot::menuPlotMapViewClicked()
{
    QScreen *scr = QApplication::screens().at(0);
    QRect rect = scr->availableGeometry();
    QSize thisDecoration = this->frameSize() - this->size();

    this->move(rect.x(), rect.y());
    this->resize(rect.width() / 2 - thisDecoration.width(), rect.height() - thisDecoration.height());

    QSize GEDecoration = mapView->frameSize() - mapView->size();
    mapView->resize(rect.width()/2 - GEDecoration.width(), rect.height() - GEDecoration.height());
    mapView->move(rect.x() + rect.width() / 2, rect.y());

    mapView->setVisible(true);
}
//---------------------------------------------------------------------------
#if 0
void Plot::DispGesture()
{
    AnsiString s;
    int b, e;

    b = EventInfo.Flags.Contains(gfBegin);
    e = EventInfo.Flags.Contains(gfEnd);

    if (EventInfo.GestureID == Controls::igiZoom) {
        s.sprintf("zoom: Location=%d,%d,Flag=%d,%d,Angle=%.1f,Disnance=%d",
              EventInfo.Location.X, EventInfo.Location.Y, b, e,
              EventInfo.Angle, EventInfo.Distance);
        Message1->Caption = s;
    } else if (EventInfo.GestureID == Controls::igiPan) {
        s.sprintf("pan: Location=%d,%d,Flag=%d,%d,Angle=%.1f,Disnance=%d",
              EventInfo.Location.X, EventInfo.Location.Y, b, e,
              EventInfo.Angle, EventInfo.Distance);
        Message1->Caption = s;
    } else if (EventInfo.GestureID == Controls::igiRotate) {
        s.sprintf("rotate: Location=%d,%d,Flag=%d,%d,Angle=%.1f,Disnance=%d",
              EventInfo.Location.X, EventInfo.Location.Y, b, e,
              EventInfo.Angle, EventInfo.Distance);
        Message1->Caption = s;
    }
}
#endif
// callback on menu-animation-start -----------------------------------------
void Plot::menuAnimationStartClicker()
{
    trace(3, "MenuAnimStartClick\n");
}
// callback on menu-animation-stop ------------------------------------------
void Plot::menuAnimationStopClicker()
{
    trace(3, "MenuAnimStopClick\n");
}
// callback on menu-about ---------------------------------------------------
void Plot::menuAboutClicked()
{
    trace(3, "MenuAboutClick\n");

    aboutDialog->aboutString = PRGNAME;
    aboutDialog->iconIndex = 2;
    aboutDialog->exec();
}
// callback on button-connect/disconnect ------------------------------------
void Plot::btnConnectClicked()
{
    trace(3, "BtnConnectClick\n");

    if (!connectState) menuConnectClicked();
    else menuDisconnectClicked();
}
// callback on button-solution-1 --------------------------------------------
void Plot::btnSolution1Clicked()
{
    trace(3, "btnSolution1Click\n");

    btnSolution12->setChecked(false);
    updateTime();
    updatePlot();
    updateEnable();
}
// callback on button-solution-2 --------------------------------------------
void Plot::btnSolution2Clicked()
{
    trace(3, "btnSolution2Click\n");

    btnSolution12->setChecked(false);
    updateTime();
    updatePlot();
    updateEnable();
}
// callback on button-solution-1-2 ------------------------------------------
void Plot::btnSolution12Clicked()
{
    trace(3, "btnSolution12Click\n");

    btnSolution1->setChecked(false);
    btnSolution2->setChecked(false);
    updateTime();
    updatePlot();
    updateEnable();
}
// callback on button-solution-1 double-click -------------------------------
void Plot::btnSolution1DblClicked()
{
    trace(3, "btnSolution1DblClick\n");

    menuOpenSolution1Clicked();
}
// callback on button-solution-2 double-click -------------------------------
void Plot::btnSolution2DblClicked()
{
    trace(3, "btnSolution2DblClick\n");

    menuOpenSolution2Clicked();
}

// callback on button-plot-1-onoff ------------------------------------------
void Plot::btnOn1Clicked()
{
    trace(3, "BtnOn1Click\n");

    updateSize();
    refresh();
}
// callback on button-plot-2-onoff-------------------------------------------
void Plot::btnOn2Clicked()
{
    trace(3, "BtnOn2Click\n");

    updateSize();
    refresh();
}
// callback on button-plot-3-onoff ------------------------------------------
void Plot::btnOn3Clicked()
{
    trace(3, "BtnOn3Click\n");

    updateSize();
    refresh();
}
// callback on button-range-list --------------------------------------------
void Plot::btnRangeListClicked()
{
    QToolButton *btn=(QToolButton *)sender();
    trace(3, "BtnRangeListClick\n");

    QPoint pos = btn->mapToGlobal(btn->pos());
    pos.rx() -= btn->width();
    pos.ry() += btn->height();

    lWRangeList->move(pos);
    lWRangeList->setVisible(!lWRangeList->isVisible());
}
// callback on button-range-list --------------------------------------------
void Plot::rangeListClicked()
{
    bool okay;
    QString unit;
    QListWidgetItem *i;

    trace(3, "RangeListClick\n");

    lWRangeList->setVisible(false);
    if ((i = lWRangeList->currentItem()) == NULL) return;

    QStringList tokens = i->text().split(" ");

    if (tokens.length()!=2) return;

    yRange = tokens.at(0).toDouble(&okay);
    if (!okay) return;

    unit = tokens.at(1);

    if (unit == "cm") yRange*=0.01;
    else if (unit == "km") yRange*=1000.0;
    setRange(0, yRange);
    updatePlot();
    updateEnable();
}

// callback on button-animation ---------------------------------------------
void Plot::btnAnimateClicked()
{
    trace(3, "BtnAnimateClick\n");

    updateEnable();
}
// callback on button-message 2 ---------------------------------------------
void Plot::btnMessage2Clicked()
{
    if (++pointType > 2) pointType = 0;
}
// callback on plot-type selection change -----------------------------------
void Plot::plotTypeSChanged()
{
    int i;

    trace(3, "PlotTypeSChnage\n");

    for (i = 0; !PTypes[i].isNull(); i++)
        if (cBPlotTypeS->currentText() == PTypes[i]) updateType(i);

    updateTime();
    updatePlot();
    updateEnable();
}
// callback on quality-flag selection change --------------------------------
void Plot::qFlagChanged()
{
    trace(3, "QFlagChange\n");

    updatePlot();
    updateEnable();
}
// callback on obs-type selection change ------------------------------------
void Plot::observationTypeChanged()
{
    trace(3, "ObsTypeChange\n");

    updatePlot();
    updateEnable();
}
// callback on dop-type selection change ------------------------------------
void Plot::dopTypeChanged()
{
    trace(3, "DopTypeChange\n");

    updatePlot();
    updateEnable();
}
// callback on satellite-list selection change ------------------------------
void Plot::satelliteListChanged()
{
    trace(3, "SatListChange\n");

    updateSatelliteSelection();
    updatePlot();
    updateEnable();
}
// callback on time scroll-bar change ---------------------------------------
void Plot::timeScrollbarChanged()
{
    int sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0;

    trace(3, "TimeScrollChange\n");

    if (plotType <= PLOT_NSAT || plotType == PLOT_RES)
        solutionIndex[sel] = sBTime->value();
    else
        observationIndex = sBTime->value();
    updatePlot();
}
// callback on mouse-down event ---------------------------------------------
void Plot::mousePressEvent(QMouseEvent *event)
{
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    X0 = lblDisplay->mapFromGlobal(event->globalPosition()).x();
    Y0 = lblDisplay->mapFromGlobal(event->globalPosition()).y();
#else
    X0 = lblDisplay->mapFromGlobal(event->globalPos()).x();
    Y0 = lblDisplay->mapFromGlobal(event->globalPos()).y();
#endif
    Xcent0 = Xcent;

    trace(3, "DispMouseDown: X=%d Y=%d\n", X0, Y0);

    Drag = event->buttons().testFlag(Qt::LeftButton) ? 1 : (event->buttons().testFlag(Qt::RightButton) ? 11 : 0);

    if (plotType == PLOT_TRK)
        mouseDownTrack(X0, Y0);
    else if (plotType <= PLOT_NSAT || plotType == PLOT_RES || plotType == PLOT_SNR)
        mouseDownSolution(X0, Y0);
    else if (plotType == PLOT_OBS || plotType == PLOT_DOP)
        mouseDownObservation(X0, Y0);
    else Drag = 0;

    lWRangeList->setVisible(false);
}
// callback on mouse-move event ---------------------------------------------
void Plot::mouseMove(QMouseEvent *event)
{
    double dx, dy, dxs, dys;

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    if ((abs(lblDisplay->mapFromGlobal(event->globalPosition()).x() - Xn) < 1) &&
        (abs(lblDisplay->mapFromGlobal(event->globalPosition()).y() - Yn) < 1)) return;

    Xn = lblDisplay->mapFromGlobal(event->globalPosition()).x();
    Yn = lblDisplay->mapFromGlobal(event->globalPosition()).y();
#else
    if ((abs(lblDisplay->mapFromGlobal(event->globalPos()).x() - Xn) < 1) &&
        (abs(lblDisplay->mapFromGlobal(event->globalPos()).y() - Yn) < 1)) return;

    Xn = lblDisplay->mapFromGlobal(event->globalPos()).x();
    Yn = lblDisplay->mapFromGlobal(event->globalPos()).y();
#endif
    trace(4, "DispMouseMove: X=%d Y=%d\n", Xn, Yn);

    if (Drag == 0) {
        updatePoint(Xn, Yn);
        return;
    }

    dx = (X0 - Xn) * Xs;
    dy = (Yn - Y0) * Ys;
    dxs = pow(2.0, (X0 - Xn) / 100.0);
    dys = pow(2.0, (Yn - Y0) / 100.0);

    if (plotType == PLOT_TRK)
        mouseMoveTrack(Xn, Yn, dx, dy, dxs, dys);
    else if (plotType <= PLOT_NSAT || plotType == PLOT_RES || plotType == PLOT_SNR)
        mouseMoveSolution(Xn, Yn, dx, dy, dxs, dys);
    else if (plotType == PLOT_OBS || plotType == PLOT_DOP)
        mouseMoveObservation(Xn, Yn, dx, dy, dxs, dys);
}
// callback on mouse-up event -----------------------------------------------
void Plot::mouseReleaseEvent(QMouseEvent *event)
{
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    trace(3, "DispMouseUp: X=%d Y=%d\n", lblDisplay->mapFromGlobal(event->globalPosition()).x(), lblDisplay->mapFromGlobal(event->globalPosition()).y());
#else
    trace(3, "DispMouseUp: X=%d Y=%d\n", lblDisplay->mapFromGlobal(event->globalPos()).x(), lblDisplay->mapFromGlobal(event->globalPos()).y());
#endif
    Drag = 0;
    setCursor(Qt::ArrowCursor);
    refresh();
    refresh_MapView();
}
// callback on mouse-double-click -------------------------------------------
void Plot::mouseDoubleClickEvent(QMouseEvent *event)
{
    QPoint p(static_cast<int>(X0), static_cast<int>(Y0));
    double x, y;

    if (event->button() != Qt::LeftButton) return;

    trace(3, "DispDblClick X=%d Y=%d\n", p.x(), p.y());

    if (btnFixHorizontal->isChecked()) return;

    if (plotType == PLOT_TRK) {
        graphT->toPos(p, x, y);
        graphT->setCenter(x, y);
        refresh();
        refresh_MapView();
    } else if (plotType <= PLOT_NSAT || plotType == PLOT_RES || plotType == PLOT_SNR) {
        graphG[0]->toPos(p, x, y);
        setCenterX(x);
        refresh();
    } else if (plotType == PLOT_OBS || plotType == PLOT_DOP) {
        graphR->toPos(p, x, y);
        setCenterX(x);
        refresh();
    }
}
// callback on mouse-leave event --------------------------------------------
void Plot::leaveEvent(QEvent *)
{
    trace(3, "DispMouseLeave\n");

    Xn = Yn = -1;
    lblMessage2->setVisible(false);
    lblMessage2->setText("");
}
// callback on mouse-down event on track-plot -------------------------------
void Plot::mouseDownTrack(int X, int Y)
{
    int i, sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0;

    trace(3, "MouseDownTrk: X=%d Y=%d\n", X, Y);

    if (Drag == 1 && (i = searchPosition(X, Y)) >= 0) {
        solutionIndex[sel] = i;
        updateTime();
        updateInfo();
        Drag = 0;
        refresh();
    } else {
        graphT->getCenter(Xc, Yc);
        graphT->getScale(Xs, Ys);
        setCursor(Drag == 1 ? Qt::SizeAllCursor : Qt::SplitVCursor);
    }
}
// callback on mouse-down event on solution-plot ----------------------------
void Plot::mouseDownSolution(int X, int Y)
{
    QPushButton *btn[] = { btnOn1, btnOn2, btnOn3 };
    QPoint pnt, p(X, Y);
    gtime_t time = { 0, 0 };
    sol_t *data;
    double x, xl[2], yl[2];
    int i, area = -1, sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0;

    trace(3, "MouseDownSol: X=%d Y=%d\n", X, Y);

    if (plotType == PLOT_SNR) {
        if (0 <= observationIndex && observationIndex < nObservation) time = observation.data[indexObservation[observationIndex]].time;
    } else {
        if ((data = getsol(solutionData + sel, solutionIndex[sel]))) time = data->time;
    }
    if (time.time && !MenuFixHoriz->isChecked()) {
        x = timePosition(time);

        graphG[0]->getLimits(xl, yl);
        graphG[0]->toPoint(x, yl[1], pnt);

        if ((X - pnt.x()) * (X - pnt.x()) + (Y - pnt.y()) * (Y - pnt.y()) < 25) {
            setCursor(Qt::SizeHorCursor);
            Drag = 20;
            refresh();
            return;
        }
    }
    for (i = 0; i < 3; i++) {
        if (!btn[i]->isChecked() || (i != 1 && plotType == PLOT_SNR)) continue;

        graphG[i]->getCenter(Xc, Yc);
        graphG[i]->getScale(Xs, Ys);
        area = graphG[i]->onAxis(lblDisplay->mapFromGlobal(p));

        if (Drag == 1 && area == 0) {
            setCursor(Qt::SizeAllCursor);
            Drag += i;
            return;
        } else if (area == 1) {
            setCursor(Drag == 1 ? Qt::SizeVerCursor : Qt::SplitVCursor);
            Drag += i + 4;
            return;
        } else if (area == 0) {
            break;
        }
    }
    if (area == 0 || area == 8) {
        setCursor(Drag == 1 ? Qt::SizeHorCursor : Qt::SplitHCursor);
        Drag += 3;
    } else {
        Drag = 0;
    }
}
// callback on mouse-down event on observation-data-plot --------------------
void Plot::mouseDownObservation(int X, int Y)
{
    QPoint pnt, p(X, Y);
    double x, xl[2], yl[2];
    int area;

    trace(3, "MouseDownObs: X=%d Y=%d\n", X, Y);

    if (0 <= observationIndex && observationIndex < nObservation && !MenuFixHoriz->isChecked()) {
        x = timePosition(observation.data[indexObservation[observationIndex]].time);

        graphR->getLimits(xl, yl);
        graphR->toPoint(x, yl[1], pnt);

        if ((X - pnt.x()) * (X - pnt.x()) + (Y - pnt.y()) * (Y - pnt.y()) < 25) {
            setCursor(Qt::SizeHorCursor);
            Drag = 20;
            refresh();
            return;
        }
    }
    graphR->getCenter(Xc, Yc);
    graphR->getScale(Xs, Ys);
    area = graphR->onAxis(lblDisplay->mapFromGlobal(p));

    if (area == 0 || area == 8) {
        setCursor(Drag == 1 ? Qt::SizeHorCursor : Qt::SplitHCursor);
        Drag += 3;
    } else {
        Drag = 0;
    }
}
// callback on mouse-move event on track-plot -------------------------------
void Plot::mouseMoveTrack(int X, int Y, double dx, double dy,
            double dxs, double dys)
{
    trace(4, "MouseMoveTrk: X=%d Y=%d\n", X, Y);

    Q_UNUSED(dxs);

    if (Drag == 1 && !MenuFixHoriz->isChecked())
        graphT->setCenter(Xc + dx, Yc + dy);
    else if (Drag > 1)
        graphT->setScale(Xs * dys, Ys * dys);
    MenuCenterOri->setChecked(false);

    if (updateTimer.elapsed() < RefreshTime) return;
    updateTimer.restart();

    refresh();
}
// callback on mouse-move event on solution-plot ----------------------------
void Plot::mouseMoveSolution(int X, int Y, double dx, double dy,
            double dxs, double dys)
{
    QPoint p1, p2, p(X, Y);
    double x, y, xs, ys;
    int i, sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0;

    trace(4, "MouseMoveSol: X=%d Y=%d\n", X, Y);

    if (Drag <= 4) {
        for (i = 0; i < 3; i++) {
            graphG[i]->getCenter(x, y);
            if (!MenuFixHoriz->isChecked())
                x = Xc + dx;
            if (!MenuFixVert->isChecked() || !MenuFixVert->isEnabled())
                y = i == Drag - 1 ? Yc + dy : y;
            graphG[i]->setCenter(x, y);
            setCenterX(x);
        }
        if (MenuFixHoriz->isChecked()) {
            graphG[0]->getPosition(p1, p2);
            Xcent = Xcent0 + 2.0 * (X - X0) / (p2.x() - p1.x());
            if (Xcent > 1.0) Xcent = 1.0;
            if (Xcent < -1.0) Xcent = -1.0;
        }
    } else if (Drag <= 7) {
        graphG[Drag - 5]->getCenter(x, y);
        if (!MenuFixVert->isChecked() || !MenuFixVert->isEnabled())
            y = Yc + dy;
        graphG[Drag - 5]->setCenter(x, y);
    } else if (Drag <= 14) {
        for (i = 0; i < 3; i++) {
            graphG[i]->getScale(xs, ys);
            graphG[i]->setScale(Xs * dxs, ys);
        }
        setScaleX(Xs * dxs);
    } else if (Drag <= 17) {
        graphG[Drag - 15]->getScale(xs, ys);
        graphG[Drag - 15]->setScale(xs, Ys * dys);
    } else if (Drag == 20) {
        graphG[0]->toPos(p, x, y);
        if (plotType == PLOT_SNR) {
            for (i = 0; i < nObservation; i++)
                if (timePosition(observation.data[indexObservation[i]].time) >= x) break;
            observationIndex = i < nObservation ? i : nObservation - 1;
        } else {
            for (i = 0; i < solutionData[sel].n; i++)
                if (timePosition(solutionData[sel].data[i].time) >= x) break;
            solutionIndex[sel] = i < solutionData[sel].n ? i : solutionData[sel].n - 1;
        }
        updateTime();
    }
    MenuCenterOri->setChecked(false);

    if (updateTimer.elapsed() < RefreshTime) return;
    updateTimer.restart();

    refresh();
}
// callback on mouse-move events on observataion-data-plot ------------------
void Plot::mouseMoveObservation(int X, int Y, double dx, double dy,
            double dxs, double dys)
{
    QPoint p1, p2, p(X, Y);
    double x, y, xs, ys;
    int i;

    Q_UNUSED(dys);

    trace(4, "MouseMoveObs: X=%d Y=%d\n", X, Y);

    if (Drag <= 4) {
        graphR->getCenter(x, y);
        if (!MenuFixHoriz->isChecked()) x = Xc + dx;
        if (!MenuFixVert->isChecked()) y = Yc + dy;
        graphR->setCenter(x, y);
        setCenterX(x);

        if (MenuFixHoriz->isChecked()) {
            graphR->getPosition(p1, p2);
            Xcent = Xcent0 + 2.0 * (X - X0) / (p2.x() - p1.x());
            if (Xcent > 1.0) Xcent = 1.0;
            if (Xcent < -1.0) Xcent = -1.0;
        }
    } else if (Drag <= 14) {
        graphR->getScale(xs, ys);
        graphR->setScale(Xs * dxs, ys);
        setScaleX(Xs * dxs);
    } else if (Drag == 20) {
        graphR->toPos(p, x, y);
        for (i = 0; i < nObservation; i++)
            if (timePosition(observation.data[indexObservation[i]].time) >= x) break;
        observationIndex = i < nObservation ? i : nObservation - 1;
        updateTime();
    }
    MenuCenterOri->setChecked(false);

    if (updateTimer.elapsed() < RefreshTime) return;
    updateTimer.restart();

    refresh();
}
// callback on mouse-wheel events -------------------------------------------
void Plot::wheelEvent(QWheelEvent *event)
{
    QPoint p(Xn, Yn);
    double xs, ys, ds = pow(2.0, -event->angleDelta().y() / 1200.0);
    int i, area = -1;

    event->accept();

    trace(4, "MouseWheel: WheelDelta=%d\n", event->angleDelta().y());

    if (Xn < 0 || Yn < 0) return;

    if (plotType == PLOT_TRK) { // track-plot
        graphT->getScale(xs, ys);
        graphT->setScale(xs * ds, ys * ds);
    } else if (plotType <= PLOT_NSAT || plotType == PLOT_RES || plotType == PLOT_SNR) {
        for (i = 0; i < 3; i++) {
            if (plotType == PLOT_SNR && i != 1) continue;
            area = graphG[i]->onAxis(p);
            if (area == 0 || area == 1 || area == 2) {
                graphG[i]->getScale(xs, ys);
                graphG[i]->setScale(xs, ys * ds);
            } else if (area == 0) {
                break;
            }
        }
        if (area == 8) {
            for (i = 0; i < 3; i++) {
                graphG[i]->getScale(xs, ys);
                graphG[i]->setScale(xs * ds, ys);
                setScaleX(xs * ds);
            }
        }
    } else if (plotType == PLOT_OBS || plotType == PLOT_DOP) {
        area = graphR->onAxis(p);
        if (area == 0 || area == 8) {
            graphR->getScale(xs, ys);
            graphR->setScale(xs * ds, ys);
            setScaleX(xs * ds);
        }
    } else {
        return;
    }

    refresh();
}
// callback on key-down events ----------------------------------------------
void Plot::keyPressEvent(QKeyEvent *event)
{
    double sfact = 1.05, fact = event->modifiers().testFlag(Qt::ShiftModifier) ? 1.0 : 10.0;
    double xc, yc, yc1, yc2, yc3, xs, ys, ys1, ys2, ys3;
    int key = event->modifiers().testFlag(Qt::ControlModifier) ? 10 : 0;

    trace(3, "FormKeyDown:\n");

    switch (event->key()) {
    case Qt::Key_Up: key += 1; break;
    case Qt::Key_Down: key += 2; break;
    case Qt::Key_Left: key += 3; break;
    case Qt::Key_Right: key += 4; break;
    default: return;
    }
    if (event->modifiers().testFlag(Qt::AltModifier)) return;

    if (plotType == PLOT_TRK) {
        graphT->getCenter(xc, yc);
        graphT->getScale(xs, ys);
        if (key == 1) {
            if (!MenuFixHoriz->isChecked()) yc += fact * ys;
        }
        if (key == 2) {
            if (!MenuFixHoriz->isChecked()) yc -= fact * ys;
        }
        if (key == 3) {
            if (!MenuFixHoriz->isChecked()) xc -= fact * xs;
        }
        if (key == 4) {
            if (!MenuFixHoriz->isChecked()) xc += fact * xs;
        }
        if (key == 11) {
            xs /= sfact; ys /= sfact;
        }
        if (key == 12) {
            xs *= sfact; ys *= sfact;
        }
        graphT->setCenter(xc, yc);
        graphT->setScale(xs, ys);
    } else if (plotType <= PLOT_NSAT || plotType == PLOT_RES) {
        graphG[0]->getCenter(xc, yc1);
        graphG[1]->getCenter(xc, yc2);
        graphG[2]->getCenter(xc, yc3);
        graphG[0]->getScale(xs, ys1);
        graphG[1]->getScale(xs, ys2);
        graphG[2]->getScale(xs, ys3);
        if (key == 1) {
            if (!MenuFixVert->isChecked()) yc1 += fact * ys1;
            yc2 += fact * ys2;
            yc3 += fact * ys3;
        }
        if (key == 2) {
            if (!MenuFixVert->isChecked()) yc1 -= fact * ys1;
            yc2 -= fact * ys2;
            yc3 -= fact * ys3;
        }
        if (key == 3) {
            if (!MenuFixHoriz->isChecked()) xc -= fact * xs;
        }
        if (key == 4) {
            if (!MenuFixHoriz->isChecked()) xc += fact * xs;
        }
        if (key == 11) {
            ys1 /= sfact;
            ys2 /= sfact;
            ys3 /= sfact;
        }
        if (key == 12) {
            ys1 *= sfact;
            ys2 *= sfact;
            ys3 *= sfact;
        }
        if (key == 13) xs *= sfact;
        if (key == 14) xs /= sfact;
        graphG[0]->setCenter(xc, yc1);
        graphG[1]->setCenter(xc, yc2);
        graphG[2]->setCenter(xc, yc3);
        graphG[0]->setScale(xs, ys1);
        graphG[1]->setScale(xs, ys2);
        graphG[2]->setScale(xs, ys3);
    } else if (plotType == PLOT_OBS || plotType == PLOT_DOP || plotType == PLOT_SNR) {
        graphR->getCenter(xc, yc);
        graphR->getScale(xs, ys);
        if (key == 1) {
            if (!MenuFixVert->isChecked()) yc += fact * ys;
        }
        if (key == 2) {
            if (!MenuFixVert->isChecked()) yc -= fact * ys;
        }
        if (key == 3) {
            if (!MenuFixHoriz->isChecked()) xc -= fact * xs;
        }
        if (key == 4) {
            if (!MenuFixHoriz->isChecked()) xc += fact * xs;
        }
        if (key == 11) ys /= sfact;
        if (key == 12) xs *= sfact;
        if (key == 13) xs *= sfact;
        if (key == 14) xs /= sfact;
        graphR->setCenter(xc, yc);
        graphR->setScale(xs, ys);
    }
    refresh();
}
// callback on interval-timer -----------------------------------------------
void Plot::timerTimer()
{
    const QColor color[] = { Qt::red, Qt::gray, QColor(0x00, 0xAA, 0xFF), Qt::green, QColor(0x00, 0xff, 0x00) };
    QLabel *strstatus[] = { lblStreamStatus1, lblStreamStatus2 };
    Console *console[] = { console1, console2 };
    QString connectmsg = "";
    static uint8_t buff[16384];
    solopt_t opt = solopt_default;
    sol_t *sol;
    const gtime_t ts = { 0, 0 };
    gtime_t time = {0, 0};
    double tint = timeEnabled[2]?timeInterval:0.0, pos[3], ep[6];
    int i, j, n, inb, inr, cycle, nmsg[2] = { 0 }, stat, istat;
    int sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0;
    char msg[MAXSTRMSG] = "";

    trace(4, "TimeTimer\n");

    if (connectState) { // real-time input mode
        for (i = 0; i < 2; i++) {
            opt.posf = rtFormat[i];
            opt.times = rtTimeFormat == 0 ? 0 : rtTimeFormat - 1;
            opt.timef = rtTimeFormat >= 1;
            opt.degf = rtDegFormat;
            strcpy(opt.sep, qPrintable(rtFieldSeperator));
            strsum(stream + i, &inb, &inr, NULL, NULL);
            stat = strstat(stream + i, msg);
            strstatus[i]->setStyleSheet(QStringLiteral("QLabel {color %1;}").arg(color2String(color[stat < 3 ? stat + 1 : 3])));
            if (*msg && strcmp(msg, "localhost"))
                connectmsg += QStringLiteral("(%1) %2 ").arg(i + 1).arg(msg);
            while ((n = strread(stream + i, buff, sizeof(buff))) > 0) {
                for (j = 0; j < n; j++) {
                    istat = inputsol(buff[j], ts, ts, tint, 0, &opt, solutionData + i);
                    if (istat == 0) continue;
                    if (istat < 0) { // disconnect received
                        disconnectStream();
                        return;
                    }
                    if (week == 0 && solutionData[i].n == 1) { // first data
                        if (plotType > PLOT_NSAT)
                            updateType(PLOT_TRK);
                        time2gpst(solutionData[i].time, &week);
                        updateOrigin();
                        ecef2pos(solutionData[i].data[0].rr, pos);
                        mapView->setCenter(pos[0]*R2D,pos[1]*R2D);
                    }
                    nmsg[i]++;
                }
                console[i]->addMessage(buff, n);
            }
            if (nmsg[i] > 0) {
                strstatus[i]->setStyleSheet(QStringLiteral("QLabel {color %1;}").arg(color2String(color[4])));
                solutionIndex[i] = solutionData[i].n - 1;
            }
        }
        lblConnectMessage->setText(connectmsg);

        if (nmsg[0] <= 0 && nmsg[1] <= 0) return;
    } else if (btnAnimate->isEnabled() && btnAnimate->isChecked()) { // animation mode
        cycle = animationCycle <= 0 ? 1 : animationCycle;

        if (plotType <= PLOT_NSAT || plotType == PLOT_RES) {
            solutionIndex[sel] += cycle;
            if (solutionIndex[sel] >= solutionData[sel].n - 1) {
                solutionIndex[sel] = solutionData[sel].n - 1;
                btnAnimate->setChecked(false);
            }
        } else {
            observationIndex += cycle;
            if (observationIndex >= nObservation - 1) {
                observationIndex = nObservation - 1;
                btnAnimate->setChecked(false);
            }
        }
    } else if (timeSyncOut) { // time sync
        time.time = 0;
        while (strread(&streamTimeSync, (uint8_t *)streamBuffer + nStreamBuffer, 1)) {
            if (++nStreamBuffer >= 1023) {
                nStreamBuffer = 0;
                continue;
            }
            if (streamBuffer[nStreamBuffer-1] == '\n') {
                streamBuffer[nStreamBuffer-1] = '\0';
                if (sscanf(streamBuffer,"%lf/%lf/%lf %lf:%lf:%lf",ep,ep+1,ep+2,
                           ep+3,ep+4,ep+5)>=6) {
                    time=epoch2time(ep);
                }
                nStreamBuffer = 0;
            }
        }
        if (time.time && (plotType <= PLOT_NSAT || plotType <= PLOT_RES)) {
           i = solutionIndex[sel];
           if (!(sol = getsol(solutionData + sel, i))) return;
           double tt = timediff(sol->time, time);
           if (tt < -DTTOL) {
               for (;i < solutionData[sel].n; i++) {
                   if (!(sol = getsol(solutionData + sel,i))) continue;
                   if (timediff(sol->time, time) >= -DTTOL) {
                       i--;
                       break;
                   }
               }
           } else if (tt > DTTOL) {
               for (;i >= 0; i--) {
                   if (!(sol = getsol(solutionData + sel,i))) continue;
                   if (timediff(sol->time, time) <= DTTOL) break;
               }
           }
           solutionIndex[sel] = MAX(0, MIN(solutionData[sel].n - 1, i));
        }
        else return;
    }
    else {
        return;
    }

    updateTime();

    if (updateTimer.elapsed() < RefreshTime) return;
    updateTimer.restart();

    updatePlot();
}
// set center of x-axis -----------------------------------------------------
void Plot::setCenterX(double c)
{
    double x, y;
    int i;

    trace(3, "SetCentX: c=%.3f:\n", c);

    graphR->getCenter(x, y);
    graphR->setCenter(c, y);
    for (i = 0; i < 3; i++) {
        graphG[i]->getCenter(x, y);
        graphG[i]->setCenter(c, y);
    }
}
// set scale of x-axis ------------------------------------------------------
void Plot::setScaleX(double s)
{
    double xs, ys;
    int i;

    trace(3, "SetScaleX: s=%.3f:\n", s);

    graphR->getScale(xs, ys);
    graphR->setScale(s, ys);
    for (i = 0; i < 3; i++) {
        graphG[i]->getScale(xs, ys);
        graphG[i]->setScale(s, ys);
    }
}
// update plot-type with fit-range ------------------------------------------
void Plot::updateType(int type)
{
    trace(3, "UpdateType: type=%d\n", type);

    plotType = type;

    if (autoScale && plotType <= PLOT_SOLA && (solutionData[0].n > 0 || solutionData[1].n > 0))
        fitRange(0);
    else
        setRange(0, yRange);
    updatePlotType();
}
// update size of plot ------------------------------------------------------
void Plot::updateSize(void)
{
    QPushButton *btn[] = { btnOn1, btnOn2, btnOn3 };
    QPoint p1(0, 0), p2(lblDisplay->width(), lblDisplay->height());
    double xs, ys,font_px=lblDisplay->font().pixelSize()*1.33;
    int i, n, h, tmargin, bmargin, rmargin, lmargin;

    trace(3, "UpdateSize\n");

    tmargin=(int)(font_px*0.9); // top margin (px)
    bmargin=(int)(font_px*1.8); // bottom
    rmargin=(int)(font_px*1.2); // right
    lmargin=(int)(font_px*3.6); // left

    graphT->resize();
    graphS->resize();
    graphR->resize();
    for (int i = 0; i < 3; i++)
        graphG[i]->resize();
    for (int i = 0; i < 2; i++)
        graphE[i]->resize();

    graphT->setPosition(p1, p2);
    graphS->setPosition(p1, p2);
    graphS->getScale(xs, ys);
    xs = MAX(xs, ys);
    graphS->setScale(xs, xs);
    p1.rx() += lmargin; p1.ry() += tmargin;
    p2.rx() -= rmargin; p2.setY(p2.y() - bmargin);
    graphR->setPosition(p1, p2);

    p1.setX(p1.x()+(int)(font_px*1.2));
    p1.setY(tmargin); p2.setY(p1.y());
    for (i = n = 0; i < 3; i++) if (btn[i]->isChecked()) n++;
    for (i = 0; i < 3; i++) {
        if (!btn[i]->isChecked() || (n <= 0)) continue;
        if (n == 0) break;
        h = (lblDisplay->height() - tmargin - bmargin) / n;
        p2.ry() += h;
        graphG[i]->setPosition(p1, p2);
        p1.ry() += h;
    }
    p1.setY(tmargin); p2.setY(p1.y());
    for (i = n = 0; i < 2; i++) if (btn[i]->isChecked()) n++;
    for (i = 0; i < 2; i++) {
        if (!btn[i]->isChecked() || (n <= 0)) continue;
        if (n == 0) break;
        h = (lblDisplay->height() - tmargin - bmargin) / n;
        p2.ry() += h;
        graphE[i]->setPosition(p1, p2);
        p1.ry() += h;
    }
}
// update colors on plot ----------------------------------------------------
void Plot::updateColor(void)
{
    int i;

    trace(3, "UpdateColor\n");

    for (i = 0; i < 3; i++) {
        graphT->color[i] = cColor[i];
        graphR->color[i] = cColor[i];
        graphS->color[i] = cColor[i];
        graphG[0]->color[i] = cColor[i];
        graphG[1]->color[i] = cColor[i];
        graphG[2]->color[i] = cColor[i];
    }
    lblDisplay->setFont(font);
}
// update time-cursor -------------------------------------------------------
void Plot::updateTime(void)
{
    gtime_t time;
    sol_t *sol;
    double tt;
    int i, sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0;

    trace(3, "UpdateTime\n");

    // time-cursor change on solution-plot
    if (plotType <= PLOT_NSAT || plotType <= PLOT_RES) {
        sBTime->setMaximum(MAX(1, solutionData[sel].n - 1));
        sBTime->setValue(solutionIndex[sel]);
        if (!(sol = getsol(solutionData + sel, solutionIndex[sel]))) return;
        time = sol->time;
    } else if (nObservation > 0) { // time-cursor change on observation-data-plot
        sBTime->setMaximum(MAX(1, nObservation - 1));
        sBTime->setValue(observationIndex);
        time = observation.data[indexObservation[observationIndex]].time;
    } else {
        return;
    }

    // time-synchronization between solutions and observation-data
    for (sel = 0; sel < 2; sel++) {
        i = solutionIndex[sel];
        if (!(sol = getsol(solutionData + sel, i))) continue;
        tt = timediff(sol->time, time);
        if (tt < -DTTOL) {
            for (; i < solutionData[sel].n; i++) {
                if (!(sol = getsol(solutionData + sel, i))) continue;
                if (timediff(sol->time, time) >= -DTTOL) break;
            }
        } else if (tt > DTTOL) {
            for (; i >= 0; i--) {
                if (!(sol = getsol(solutionData + sel, i))) continue;
                if (timediff(sol->time, time) <= DTTOL) break;
            }
        }
        solutionIndex[sel] = MAX(0, MIN(solutionData[sel].n - 1, i));
    }
    i = observationIndex;
    if (i <= nObservation - 1) {
        tt = timediff(observation.data[indexObservation[i]].time, time);
        if (tt < -DTTOL) {
            for (; i < nObservation; i++)
                if (timediff(observation.data[indexObservation[i]].time, time) >= -DTTOL) break;
        } else if (tt > DTTOL) {
            for (; i >= 0; i--)
                if (timediff(observation.data[indexObservation[i]].time, time) <= DTTOL) break;
        }
        observationIndex = MAX(0, MIN(nObservation - 1, i));
    }
}
// update origin of plot ----------------------------------------------------
void Plot::updateOrigin(void)
{
    gtime_t time = { 0, 0 };
    sol_t *sol;
    double opos[3] = { 0 }, pos[3], ovel[3] = { 0 };
    int i, j, n = 0, sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0;
    QString sta;

    trace(3, "UpdateOrigin\n");

    if (origin == ORG_STARTPOS) {
        if (!(sol = getsol(solutionData, 0)) || sol->type != 0) return;
        for (i = 0; i < 3; i++) opos[i] = sol->rr[i];
    } else if (origin == ORG_ENDPOS) {
        if (!(sol = getsol(solutionData, solutionData[0].n - 1)) || sol->type != 0) return;
        for (i = 0; i < 3; i++) opos[i] = sol->rr[i];
    } else if (origin == ORG_AVEPOS) {
        for (i = 0; (sol = getsol(solutionData, i)) != NULL; i++) {
            if (sol->type != 0) continue;
            for (j = 0; j < 3; j++) opos[j] += sol->rr[j];
            n++;
        }
        if (n > 0) for (i = 0; i < 3; i++) opos[i] /= n;
    } else if (origin == ORG_FITPOS) {
        if (!fitPosition(&time, opos, ovel)) return;
    } else if (origin == ORG_REFPOS) {
        if (norm(solutionData[0].rb, 3) > 0.0) {
            for (i = 0; i < 3; i++) opos[i] = solutionData[0].rb[i];
        } else {
            if (!(sol = getsol(solutionData, 0)) || sol->type != 0) return;
            for (i = 0; i < 3; i++) opos[i] = sol->rr[i];
        }
    } else if (origin == ORG_LLHPOS) {
        pos2ecef(ooPosition, opos);
    } else if (origin == ORG_AUTOPOS) {
        if (solutionFiles[sel].count() > 0) {
            QFileInfo fi(solutionFiles[sel].at(0));

            readStationPosition(fi.baseName().left(4).toUpper(), sta, opos);
        }
    } else if (origin == ORG_IMGPOS) {
        pos[0] = mapLatitude * D2R;
        pos[1] = mapLongitude * D2R;
        pos[2] = 0.0;
        pos2ecef(pos, opos);
    } else if (origin == ORG_MAPPOS) {
        pos[0] = (gis.bound[0] + gis.bound[1]) / 2.0;
        pos[1] = (gis.bound[2] + gis.bound[3]) / 2.0;
        pos[2] = 0.0;
        pos2ecef(pos, opos);
    } else if (origin - ORG_PNTPOS < MAXWAYPNT) {
        for (i = 0; i < 3; i++) opos[i] = pointPosition[origin - ORG_PNTPOS][i];
    }
    if (norm(opos, 3) <= 0.0) {
        // default start position
        if (!(sol = getsol(solutionData, 0)) || sol->type != 0) return;
        for (i = 0; i < 3; i++) opos[i] = sol->rr[i];
    }
    oEpoch = time;
    for (i = 0; i < 3; i++) {
        oPosition[i] = opos[i];
        oVelocity[i] = ovel[i];
    }
    refresh_MapView();
}
// update satellite mask ----------------------------------------------------
void Plot::updateSatelliteMask(void)
{
    int sat, prn;
    char buff[256], *p;

    trace(3, "UpdateSatMask\n");

    for (sat = 1; sat <= MAXSAT; sat++) satelliteMask[sat - 1] = 0;
    for (sat = 1; sat <= MAXSAT; sat++)
        if (!(satsys(sat, &prn) & navSys)) satelliteMask[sat - 1] = 1;
    if (excludedSatellites != "") {
        strcpy(buff, qPrintable(excludedSatellites));

        for (p = strtok(buff, " "); p; p = strtok(NULL, " ")) {
            if (*p == '+' && (sat = satid2no(p + 1))) satelliteMask[sat - 1] = 0; // included
            else if ((sat = satid2no(p))) satelliteMask[sat - 1] = 1;             // excluded
        }
    }
}
// update satellite select ---------------------------------------------------
void Plot::updateSatelliteSelection(void)
{
    QString SatListText = cBSatelliteList->currentText();
    char id[16];
    int i, sys = 0;

    if (SatListText == "G") sys = SYS_GPS;
    else if (SatListText == "R") sys = SYS_GLO;
    else if (SatListText == "E") sys = SYS_GAL;
    else if (SatListText == "J") sys = SYS_QZS;
    else if (SatListText == "C") sys = SYS_CMP;
    else if (SatListText == "I") sys = SYS_IRN;
    else if (SatListText == "S") sys = SYS_SBS;

    for (i = 0; i < MAXSAT; i++) {
        satno2id(i + 1, id);
        satelliteSelection[i] = SatListText == "ALL" || SatListText == id || satsys(i + 1, NULL) == sys;
    }
}
// update enable/disable of widgets -----------------------------------------
void Plot::updateEnable(void)
{
    bool data = btnSolution1->isChecked() || btnSolution2->isChecked() || btnSolution12->isChecked();
    bool plot = (PLOT_SOLP <= plotType) && (plotType <= PLOT_NSAT);
    bool sel = (!btnSolution1->isChecked()) && (btnSolution2->isChecked()) ? 1 : 0;

    trace(3, "UpdateEnable\n");

    Panel1->setVisible(MenuToolBar->isChecked());
    Panel21->setVisible(MenuStatusBar->isChecked());

    MenuConnect->setChecked(connectState);
    btnSolution1->setEnabled(true);
    btnSolution2->setEnabled(plotType<=PLOT_NSAT||plotType==PLOT_RES||plotType==PLOT_RESE);
    btnSolution12->setEnabled(!connectState && plotType <= PLOT_SOLA && solutionData[0].n > 0 && solutionData[1].n > 0);

    cBQFlag->setVisible(plotType == PLOT_TRK || plotType == PLOT_SOLP ||
              plotType == PLOT_SOLV || plotType == PLOT_SOLA ||
              plotType == PLOT_NSAT);
    cBObservationType->setVisible(plotType == PLOT_OBS || plotType <= PLOT_SKY);
    cBObservationType2->setVisible(plotType == PLOT_SNR || plotType == PLOT_SNRE || plotType == PLOT_MPS);

    cBFrequencyType->setVisible(plotType == PLOT_RES||plotType==PLOT_RESE);
    cBDopType->setVisible(plotType == PLOT_DOP);
    cBSatelliteList->setVisible(plotType == PLOT_RES ||plotType==PLOT_RESE||plotType>=PLOT_OBS||
                plotType == PLOT_SKY || plotType == PLOT_DOP ||
                plotType == PLOT_SNR || plotType == PLOT_SNRE ||
                plotType == PLOT_MPS);
    cBQFlag->setEnabled(data);
    cBObservationType->setEnabled(data && !simObservation);
    cBObservationType2->setEnabled(data && !simObservation);

    Panel102->setVisible(plotType==PLOT_SOLP||plotType==PLOT_SOLV||
                         plotType==PLOT_SOLA||plotType==PLOT_NSAT||
                         plotType==PLOT_RES ||plotType==PLOT_RESE||
                         plotType==PLOT_SNR ||plotType==PLOT_SNRE);
    btnOn1->setEnabled(plot||plotType==PLOT_SNR||plotType==PLOT_RES||
                             plotType==PLOT_RESE||plotType==PLOT_SNRE);
    btnOn2->setEnabled(plot||plotType==PLOT_SNR||plotType==PLOT_RES||
                             plotType==PLOT_RESE||plotType==PLOT_SNRE);
    btnOn3->setEnabled(plot || plotType == PLOT_SNR || plotType == PLOT_RES);

    btnCenterOrigin->setVisible(plotType == PLOT_TRK || plotType == PLOT_SOLP ||
                 plotType == PLOT_SOLV || plotType == PLOT_SOLA ||
                 plotType == PLOT_NSAT);
    btnCenterOrigin->setEnabled(plotType != PLOT_NSAT);
    btnRangeList->setVisible(plotType == PLOT_TRK || plotType == PLOT_SOLP ||
                 plotType == PLOT_SOLV || plotType == PLOT_SOLA ||
                 plotType == PLOT_NSAT);
    btnRangeList->setEnabled(plotType != PLOT_NSAT);


    btnFitHorizontal->setVisible(plotType == PLOT_SOLP || plotType == PLOT_SOLV ||
                plotType == PLOT_SOLA || plotType == PLOT_NSAT ||
                plotType == PLOT_RES || plotType == PLOT_OBS ||
                plotType == PLOT_DOP || plotType == PLOT_SNR ||
                plotType == PLOT_SNRE);
    btnFitVertical->setVisible(plotType == PLOT_TRK || plotType == PLOT_SOLP ||
                   plotType == PLOT_SOLV || plotType == PLOT_SOLA);
    btnFitHorizontal->setEnabled(data);
    btnFitVertical->setEnabled(data);

    MenuShowTrack->setEnabled(data);
    btnFixCenter->setVisible(plotType == PLOT_TRK);
    btnFixHorizontal->setVisible(plotType == PLOT_SOLP || plotType == PLOT_SOLV ||
                plotType == PLOT_SOLA || plotType == PLOT_NSAT ||
                plotType == PLOT_RES || plotType == PLOT_OBS ||
                plotType == PLOT_DOP || plotType == PLOT_SNR);
    btnFixVertical->setVisible(plotType == PLOT_SOLP || plotType == PLOT_SOLV ||
                   plotType == PLOT_SOLA);
    btnFixCenter->setEnabled(data);
    btnFixHorizontal->setEnabled(data);
    btnFixVertical->setEnabled(data);
    btnShowMap->setVisible(plotType == PLOT_TRK);
    btnShowMap->setEnabled(!btnSolution12->isChecked());
    MenuMapView->setVisible(plotType==PLOT_TRK||plotType==PLOT_SOLP);
    Panel12->setVisible(!connectState);
    btnAnimate->setVisible(data && MenuShowTrack->isChecked());
    sBTime->setVisible(data && MenuShowTrack->isChecked());
    sBTime->setEnabled(data && MenuShowTrack->isChecked());

    if (!MenuShowTrack->isChecked()) {
        MenuFixHoriz->setEnabled(false);
        MenuFixVert->setEnabled(false);
        MenuFixCent->setEnabled(false);
        btnAnimate->setChecked(false);
    }
    MenuMapImg->setEnabled(mapImage.height() > 0);
    MenuSkyImg->setEnabled(skyImageI.height() > 0);
    MenuSrcSol->setEnabled(solutionFiles[sel].count() > 0);
    MenuSrcObs->setEnabled(observationFiles.count() > 0);
    MenuMapLayer->setEnabled(true);

    MenuShowSkyplot->setEnabled(MenuShowSkyplot->isVisible());

    btnShowImage->setVisible(plotType == PLOT_TRK || plotType == PLOT_SKY ||
                   plotType == PLOT_MPS);
    btnShowGrid->setVisible(plotType==PLOT_TRK);
    MenuAnimStart->setEnabled(!connectState && btnAnimate->isEnabled() && !btnAnimate->isChecked());
    MenuAnimStop->setEnabled(!connectState && btnAnimate->isEnabled() && btnAnimate->isChecked());

    MenuOpenSol1->setEnabled(!connectState);
    MenuOpenSol2->setEnabled(!connectState);
    MenuConnect->setEnabled(!connectState);
    MenuDisconnect->setEnabled(connectState);
    MenuPort->setEnabled(!connectState);
    MenuOpenObs->setEnabled(!connectState);
    MenuOpenNav->setEnabled(!connectState);
    MenuOpenElevMask->setEnabled(!connectState);
    MenuReload->setEnabled(!connectState);

    MenuReload->setEnabled(!connectState);
    wgStreamStatus->setEnabled(connectState);
    btnFrequency->setVisible(cBFrequencyType->isVisible()||cBObservationType->isVisible()||cBObservationType2->isVisible());
}
// linear-fitting of positions ----------------------------------------------
int Plot::fitPosition(gtime_t *time, double *opos, double *ovel)
{
    sol_t *sol;
    int i, j;
    double t, x[2], Ay[3][2] = { { 0 } }, AA[3][4] = { { 0 } };

    trace(3, "FitPos\n");

    if (solutionData[0].n <= 0) return 0;

    for (i = 0; (sol = getsol(solutionData, i)) != NULL; i++) {
        if (sol->type != 0) continue;
        if (time->time == 0) *time = sol->time;
        t = timediff(sol->time, *time);

        for (j = 0; j < 3; j++) {
            Ay[j][0] += sol->rr[j];
            Ay[j][1] += sol->rr[j] * t;
            AA[j][0] += 1.0;
            AA[j][1] += t;
            AA[j][2] += t;
            AA[j][3] += t * t;
        }
    }
    for (i = 0; i < 3; i++) {
        if (solve("N", AA[i], Ay[i], 2, 1, x)) return 0;
        opos[i] = x[0];
        ovel[i] = x[1];
    }
    return 1;
}
// fit time-range of plot ---------------------------------------------------
void Plot::fitTime(void)
{
    sol_t *sols, *sole;
    double tl[2] = { 86400.0 * 7, 0.0 }, tp[2], xl[2], yl[2], zl[2];
    int sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0;

    trace(3, "FitTime\n");

    sols = getsol(solutionData + sel, 0);
    sole = getsol(solutionData + sel, solutionData[sel].n - 1);
    if (sols && sole) {
        tl[0] = MIN(tl[0], timePosition(sols->time));
        tl[1] = MAX(tl[1], timePosition(sole->time));
    }
    if (observation.n > 0) {
        tl[0] = MIN(tl[0], timePosition(observation.data[0].time));
        tl[1] = MAX(tl[1], timePosition(observation.data[observation.n - 1].time));
    }
    if (timeEnabled[0]) tl[0] = timePosition(timeStart);
    if (timeEnabled[1]) tl[1] = timePosition(timeEnd);

    if (qFuzzyCompare(tl[0], tl[1])) {
        tl[0] = tl[0] - DEFTSPAN / 2.0;
        tl[1] = tl[0] + DEFTSPAN / 2.0;
    } else if (tl[0] > tl[1]) {
        tl[0] = -DEFTSPAN / 2.0;
        tl[1] = DEFTSPAN / 2.0;
    }
    graphG[0]->getLimits(tp, xl);
    graphG[1]->getLimits(tp, yl);
    graphG[2]->getLimits(tp, zl);
    graphG[0]->setLimits(tl, xl);
    graphG[1]->setLimits(tl, yl);
    graphG[2]->setLimits(tl, zl);
    graphR->getLimits(tp, xl);
    graphR->setLimits(tl, xl);
}
// set x/y-range of plot ----------------------------------------------------
void Plot::setRange(int all, double range)
{
    double xl[] = { -range, range };
    double yl[] = { -range, range };
    double zl[] = { -range, range };
    double xs, ys, tl[2], xp[2], pos[3];

    trace(3, "SetRange: all=%d range=%.3f\n", all, range);

    if (all || plotType == PLOT_TRK) {
        graphT->setLimits(xl, yl);
        graphT->getScale(xs, ys);
        graphT->setScale(MAX(xs, ys), MAX(xs, ys));
        if (norm(oPosition, 3) > 0.0) {
            ecef2pos(oPosition, pos);
            mapView->setCenter(pos[0] * R2D, pos[1] * R2D);
        }
    }
    if (PLOT_SOLP <= plotType && plotType <= PLOT_SOLA) {
        graphG[0]->getLimits(tl, xp);
        graphG[0]->setLimits(tl, xl);
        graphG[1]->setLimits(tl, yl);
        graphG[2]->setLimits(tl, zl);
    } else if (plotType == PLOT_NSAT) {
        graphG[0]->getLimits(tl, xp);
        xl[0] = yl[0] = zl[0] = 0.0;
        xl[1] = maxDop;
        yl[1] = YLIM_AGE;
        zl[1] = YLIM_RATIO;
        graphG[0]->setLimits(tl, xl);
        graphG[1]->setLimits(tl, yl);
        graphG[2]->setLimits(tl, zl);
    } else if (plotType < PLOT_SNR) {
        graphG[0]->getLimits(tl, xp);
        xl[0] = -maxMP; xl[1] = maxMP;
        yl[0] = -maxMP/100.0; yl[1] = maxMP/100.0;
        zl[0] = 0.0; zl[1] = 90.0;
        graphG[0]->setLimits(tl, xl);
        graphG[1]->setLimits(tl, yl);
        graphG[2]->setLimits(tl, zl);
    } else {
        graphG[0]->getLimits(tl, xp);
        xl[0] = 10.0; xl[1] = 60.0;
        yl[0] = -maxMP; yl[1] = maxMP;
        zl[0] = 0.0; zl[1] = 90.0;
        graphG[0]->setLimits(tl, xl);
        graphG[1]->setLimits(tl, yl);
        graphG[2]->setLimits(tl, zl);
    }
}
// fit x/y-range of plot ----------------------------------------------------
void Plot::fitRange(int all)
{
    TIMEPOS *pos, *pos1, *pos2;
    sol_t *data;
    double xs, ys, xp[2], tl[2], xl[] = { 1E8, -1E8 }, yl[2] = { 1E8, -1E8 }, zl[2] = { 1E8, -1E8 };
    double lat, lon, lats[2] = { 90, -90 }, lons[2] = { 180, -180 }, llh[3];
    int i, type = plotType - PLOT_SOLP;

    trace(3, "FitRange: all=%d\n", all);

    MenuFixHoriz->setChecked(false);

    if (btnSolution1->isChecked()) {
        pos = solutionToPosition(solutionData, -1, cBQFlag->currentIndex(), type);

        for (i = 0; i < pos->n; i++) {
            xl[0] = MIN(xl[0], pos->x[i]);
            yl[0] = MIN(yl[0], pos->y[i]);
            zl[0] = MIN(zl[0], pos->z[i]);
            xl[1] = MAX(xl[1], pos->x[i]);
            yl[1] = MAX(yl[1], pos->y[i]);
            zl[1] = MAX(zl[1], pos->z[i]);
        }
        delete pos;
    }
    if (btnSolution2->isChecked()) {
        pos = solutionToPosition(solutionData + 1, -1, cBQFlag->currentIndex(), type);

        for (i = 0; i < pos->n; i++) {
            xl[0] = MIN(xl[0], pos->x[i]);
            yl[0] = MIN(yl[0], pos->y[i]);
            zl[0] = MIN(zl[0], pos->z[i]);
            xl[1] = MAX(xl[1], pos->x[i]);
            yl[1] = MAX(yl[1], pos->y[i]);
            zl[1] = MAX(zl[1], pos->z[i]);
        }
        delete pos;
    }
    if (btnSolution12->isChecked()) {
        pos1 = solutionToPosition(solutionData, -1, 0, type);
        pos2 = solutionToPosition(solutionData + 1, -1, 0, type);
        pos = pos1->diff(pos2, cBQFlag->currentIndex());

        for (i = 0; i < pos->n; i++) {
            xl[0] = MIN(xl[0], pos->x[i]);
            yl[0] = MIN(yl[0], pos->y[i]);
            zl[0] = MIN(zl[0], pos->z[i]);
            xl[1] = MAX(xl[1], pos->x[i]);
            yl[1] = MAX(yl[1], pos->y[i]);
            zl[1] = MAX(zl[1], pos->z[i]);
        }
        delete pos1;
        delete pos2;
        delete pos;
    }
    xl[0] -= 0.05;
    xl[1] += 0.05;
    yl[0] -= 0.05;
    yl[1] += 0.05;
    zl[0] -= 0.05;
    zl[1] += 0.05;

    if (all || plotType == PLOT_TRK) {
        graphT->setLimits(xl, yl);
        graphT->getScale(xs, ys);
        graphT->setScale(MAX(xs, ys), MAX(xs, ys));
    }
    if (all || plotType <= PLOT_SOLA || plotType == PLOT_RES) {
        graphG[0]->getLimits(tl, xp);
        graphG[0]->setLimits(tl, xl);
        graphG[1]->setLimits(tl, yl);
        graphG[2]->setLimits(tl, zl);
    }
    if (all) {
        if (btnSolution1->isChecked()) {
            for (i = 0; (data = getsol(solutionData, i)) != NULL; i++) {
                ecef2pos(data->rr, llh);
                lats[0] = MIN(lats[0], llh[0] * R2D);
                lons[0] = MIN(lons[0], llh[1] * R2D);
                lats[1] = MAX(lats[1], llh[0] * R2D);
                lons[1] = MAX(lons[1], llh[1] * R2D);
            }
        }
        if (btnSolution2->isChecked()) {
            for (i = 0; (data = getsol(solutionData + 1, i)) != NULL; i++) {
                ecef2pos(data->rr, llh);
                lats[0] = MIN(lats[0], llh[0] * R2D);
                lons[0] = MIN(lons[0], llh[1] * R2D);
                lats[1] = MAX(lats[1], llh[0] * R2D);
                lons[1] = MAX(lons[1], llh[1] * R2D);
            }
        }
        if (lats[0] <= lats[1] && lons[0] <= lons[1]) {
            lat = (lats[0] + lats[1]) / 2.0;
            lon = (lons[0] + lons[1]) / 2.0;
        }
    }
}
// set center of track plot -------------------------------------------------
void Plot::setTrackCenter(double lat, double lon)
{
    gtime_t time = { 0, 0 };
    double pos[3] = { 0 }, rr[3], xyz[3];

    if (plotType != PLOT_TRK) return;
    pos[0] = lat * D2R;
    pos[1] = lon * D2R;
    pos2ecef(pos, rr);
    positionToXyz(time, rr, 0, xyz);
    graphT->setCenter(xyz[0], xyz[1]);
    updatePlot();
}
// load options from ini-file -----------------------------------------------
void Plot::loadOptions(void)
{
    QSettings settings(iniFile, QSettings::IniFormat);

    trace(3, "LoadOpt\n");

//    PlotType = settings.value("plot/plottype", 0).toInt();
    timeLabel = settings.value("plot/timelabel", 1).toInt();
    latLonFormat = settings.value("plot/latlonfmt", 0).toInt();
    autoScale = settings.value("plot/autoscale", 1).toInt();
    showStats = settings.value("plot/showstats", 0).toInt();
    showLabel = settings.value("plot/showlabel", 1).toInt();
    showGridLabel = settings.value("plot/showglabel", 1).toInt();
    showCompass = settings.value("plot/showcompass", 0).toInt();
    showScale = settings.value("plot/showscale", 1).toInt();
    showArrow = settings.value("plot/showarrow", 0).toInt();
    showSlip = settings.value("plot/showslip", 0).toInt();
    showHalfC = settings.value("plot/showhalfc", 0).toInt();
    showError = settings.value("plot/showerr", 0).toInt();
    showEphemeris = settings.value("plot/showeph", 0).toInt();
    plotStyle = settings.value("plot/plotstyle", 0).toInt();
    markSize = settings.value("plot/marksize", 2).toInt();
    navSys = settings.value("plot/navsys", SYS_ALL).toInt();
    animationCycle = settings.value("plot/animcycle", 10).toInt();
    refreshCycle = settings.value("plot/refcycle", 100).toInt();
    hideLowSatellites = settings.value("plot/hidelowsat", 0).toInt();
    elevationMaskP = settings.value("plot/elmaskp", 0).toInt();
    excludedSatellites = settings.value("plot/exsats", "").toString();
    rtBufferSize = settings.value("plot/rtbuffsize", 10800).toInt();
    timeSyncOut = settings.value("plot/timesyncout", 0).toInt();
    timeSyncPort = settings.value("plot/timesyncport", 10071).toInt();
    rtStream[0] = settings.value("plot/rtstream1", 0).toInt();
    rtStream[1] = settings.value("plot/rtstream2", 0).toInt();
    rtFormat[0] = settings.value("plot/rtformat1", 0).toInt();
    rtFormat[1] = settings.value("plot/rtformat2", 0).toInt();
    rtTimeFormat = settings.value("plot/rttimeform", 0).toInt();
    rtDegFormat = settings.value("plot/rtdegform", 0).toInt();
    rtFieldSeperator = settings.value("plot/rtfieldsep", "").toString();
    rtTimeoutTime = settings.value("plot/rttimeouttime", 0).toInt();
    rtReconnectTime = settings.value("plot/rtreconntime", 10000).toInt();

    mColor[0][0] = settings.value("plot/mcolor0", QColor(0xc0, 0xc0, 0xc0)).value<QColor>();
    mColor[0][1] = settings.value("plot/mcolor1", QColor(Qt::green)).value<QColor>();
    mColor[0][2] = settings.value("plot/mcolor2", QColor(0x00, 0xAA, 0xFF)).value<QColor>();
    mColor[0][3] = settings.value("plot/mcolor3", QColor(0xff, 0x00, 0xff)).value<QColor>();
    mColor[0][4] = settings.value("plot/mcolor4", QColor(Qt::blue)).value<QColor>();
    mColor[0][5] = settings.value("plot/mcolor5", QColor(Qt::red)).value<QColor>();
    mColor[0][6] = settings.value("plot/mcolor6", QColor(0x80, 0x80, 0x00)).value<QColor>();
    mColor[0][7] = settings.value("plot/mcolor7", QColor(Qt::gray)).value<QColor>();
    mColor[1][0] = settings.value("plot/mcolor8", QColor(0xc0, 0xc0, 0xc0)).value<QColor>();
    mColor[1][1] = settings.value("plot/mcolor9", QColor(0x80, 0x40, 0x00)).value<QColor>();
    mColor[1][2] = settings.value("plot/mcolor10", QColor(0x00, 0x80, 0x80)).value<QColor>();
    mColor[1][3] = settings.value("plot/mcolor11", QColor(0xFF, 0x00, 0x80)).value<QColor>();
    mColor[1][4] = settings.value("plot/mcolor12", QColor(0xFF, 0x80, 0x00)).value<QColor>();
    mColor[1][5] = settings.value("plot/mcolor13", QColor(0x80, 0x80, 0xFF)).value<QColor>();
    mColor[1][6] = settings.value("plot/mcolor14", QColor(0xFF, 0x80, 0x80)).value<QColor>();
    mColor[1][7] = settings.value("plot/mcolor15", QColor(Qt::gray)).value<QColor>();
    mapColor[0] = settings.value("plot/mapcolor1", QColor(0xc0, 0xc0, 0xc0)).value<QColor>();
    mapColor[1] = settings.value("plot/mapcolor2", QColor(0xc0, 0xc0, 0xc0)).value<QColor>();
    mapColor[2] = settings.value("plot/mapcolor3", QColor(0xf0, 0xd0, 0xd0)).value<QColor>();
    mapColor[3] = settings.value("plot/mapcolor4", QColor(0xd0, 0xf0, 0xd0)).value<QColor>();
    mapColor[4] = settings.value("plot/mapcolor5", QColor(0xd0, 0xd0, 0xf0)).value<QColor>();
    mapColor[5] = settings.value("plot/mapcolor6", QColor(0xd0, 0xf0, 0xf0)).value<QColor>();
    mapColor[6] = settings.value("plot/mapcolor7", QColor(0xf8, 0xf8, 0xd0)).value<QColor>();
    mapColor[7] = settings.value("plot/mapcolor8", QColor(0xf0, 0xf0, 0xf0)).value<QColor>();
    mapColor[8] = settings.value("plot/mapcolor9", QColor(0xf0, 0xf0, 0xf0)).value<QColor>();
    mapColor[9] = settings.value("plot/mapcolor10", QColor(0xf0, 0xf0, 0xf0)).value<QColor>();
    mapColor[10] = settings.value("plot/mapcolor11", QColor(Qt::white)).value<QColor>();
    mapColor[11] = settings.value("plot/mapcolor12", QColor(Qt::white)).value<QColor>();
    mapColorF[0] = settings.value("plot/mapcolorf1", QColor(Qt::white)).value<QColor>();
    mapColorF[1] = settings.value("plot/mapcolorf2", QColor(Qt::white)).value<QColor>();
    mapColorF[2] = settings.value("plot/mapcolorf3", QColor(Qt::white)).value<QColor>();
    mapColorF[3] = settings.value("plot/mapcolorf4", QColor(Qt::white)).value<QColor>();
    mapColorF[4] = settings.value("plot/mapcolorf5", QColor(Qt::white)).value<QColor>();
    mapColorF[5] = settings.value("plot/mapcolorf6", QColor(Qt::white)).value<QColor>();
    mapColorF[6] = settings.value("plot/mapcolorf7", QColor(Qt::white)).value<QColor>();
    mapColorF[7] = settings.value("plot/mapcolorf8", QColor(Qt::white)).value<QColor>();
    mapColorF[8] = settings.value("plot/mapcolorf9", QColor(Qt::white)).value<QColor>();
    mapColorF[9] = settings.value("plot/mapcolorf10", QColor(Qt::white)).value<QColor>();
    mapColorF[10] = settings.value("plot/mapcolorf11", QColor(Qt::white)).value<QColor>();
    mapColorF[11] = settings.value("plot/mapcolorf12", QColor(Qt::white)).value<QColor>();
    cColor[0] = settings.value("plot/color1", QColor(Qt::white)).value<QColor>();
    cColor[1] = settings.value("plot/color2", QColor(0xc0, 0xc0, 0xc0)).value<QColor>();
    cColor[2] = settings.value("plot/color3", QColor(Qt::black)).value<QColor>();
    cColor[3] = settings.value("plot/color4", QColor(0xc0, 0xc0, 0xc0)).value<QColor>();

    plotOptDialog->refDialog->stationPositionFile = settings.value("plot/staposfile", "").toString();

    elevationMask = settings.value("plot/elmask", 0.0).toDouble();
    maxDop = settings.value("plot/maxdop", 30.0).toDouble();
    maxMP = settings.value("plot/maxmp", 10.0).toDouble();
    yRange = settings.value("plot/yrange", 5.0).toDouble();
    origin = settings.value("plot/orgin", 2).toInt();
    receiverPosition = settings.value("plot/rcvpos", 0).toInt();
    ooPosition[0] = settings.value("plot/oopos1", 0).toDouble();
    ooPosition[1] = settings.value("plot/oopos2", 0).toDouble();
    ooPosition[2] = settings.value("plot/oopos3", 0).toDouble();
    shapeFile = settings.value("plot/shapefile", "").toString();
    tleFile = settings.value("plot/tlefile", "").toString();
    tleSatelliteFile = settings.value("plot/tlesatfile", "").toString();

    fontName = settings.value("plot/fontname","Tahoma").toString();
    fontSize = settings.value("plot/fontsize",8).toInt();
    font.setFamily(fontName);
    font.setPointSize(fontSize);

    rinexOptions = settings.value("plot/rnxopts", "").toString();

    mapApi = settings.value("mapview/mapapi" , 0).toInt();
    apiKey = settings.value("mapview/apikey" ,"").toString();
    mapStreams[0][0] = settings.value("mapview/mapstrs_0_0","OpenStreetMap").toString();
    mapStreams[0][1] = settings.value("mapview/mapstrs_0_1","https://tile.openstreetmap.org/{z}/{x}/{y}.png").toString();
    mapStreams[0][2] = settings.value("mapview/mapstrs_0_2","https://osm.org/copyright").toString();
    for (int i=1;i<6;i++) for (int j=0;j<3;j++) {
        mapStreams[i][j] = settings.value(QString("mapview/mapstrs_%1_%2").arg(i).arg(j),"").toString();
    }
    for (int i=0;i<2;i++) {
        streamCommands  [0][i] = settings.value(QString("str/strcmd1_%1").arg(i), "").toString();
        streamCommands  [1][i] = settings.value(QString("str/strcmd2_%1").arg(i), "").toString();
        streamCommandEnabled[0][i] = settings.value(QString("str/strcmdena1_%1").arg(i), 0).toInt();
        streamCommandEnabled[1][i] = settings.value(QString("str/strcmdena2_%1").arg(i), 0).toInt();
    }
    for (int i = 0; i < 3; i++) {
        streamPaths[0][i] = settings.value(QString("str/strpath1_%1").arg(i), "").toString();
        streamPaths[1][i] = settings.value(QString("str/strpath2_%1").arg(i), "").toString();
    }
    for (int i = 0; i < 10; i++) {
        streamHistory [i] = settings.value(QString("str/strhistry_%1").arg(i), "").toString();
    }

    TextViewer::colorText = settings.value("viewer/color1", QColor(Qt::black)).value<QColor>();
    TextViewer::colorBackground = settings.value("viewer/color2", QColor(Qt::white)).value<QColor>();
    TextViewer::font.setFamily(settings.value("viewer/fontname", "Consolas").toString());
    TextViewer::font.setPointSize(settings.value("viewer/fontsize", 9).toInt());

    MenuBrowse->setChecked(settings.value("solbrows/show",       0).toBool());
    BrowseSplitter->restoreState(settings.value("solbrows/split1",   100).toByteArray());
    directory  =settings.value("solbrows/dir",  "C:\\").toString();

    for (int i = 0; i < lWRangeList->count(); i++) {
        QString s=lWRangeList->item(i)->text();
        double range;
        QString unit;
        bool ok;

        QStringList tokens = s.split(' ');
        if (tokens.size()==2) {
            range = tokens.at(0).toInt(&ok);
            unit = tokens.at(1);
            if (ok) {
                if (unit == "cm") range*=0.01;
                else if (unit == "km") range*=1000.0;
                if (range==yRange) {
                    lWRangeList->item(i)->setSelected(true);
                    break;
                }

            }
        }

    }
}
// save options to ini-file -------------------------------------------------
void Plot::saveOption(void)
{
    QSettings settings(iniFile, QSettings::IniFormat);

    trace(3, "SaveOpt\n");

    //settings.setValue("plot/plottype", PlotType);
    settings.setValue("plot/timelabel", timeLabel);
    settings.setValue("plot/latlonfmt", latLonFormat);
    settings.setValue("plot/autoscale", autoScale);
    settings.setValue("plot/showstats", showStats);
    settings.setValue("plot/showlabel", showLabel);
    settings.setValue("plot/showglabel", showGridLabel);
    settings.setValue("plot/showcompass", showCompass);
    settings.setValue("plot/showscale", showScale);
    settings.setValue("plot/showarrow", showArrow);
    settings.setValue("plot/showslip", showSlip);
    settings.setValue("plot/showhalfc", showHalfC);
    settings.setValue("plot/showerr", showError);
    settings.setValue("plot/showeph", showEphemeris);
    settings.setValue("plot/plotstyle", plotStyle);
    settings.setValue("plot/marksize", markSize);
    settings.setValue("plot/navsys", navSys);
    settings.setValue("plot/animcycle", animationCycle);
    settings.setValue("plot/refcycle", refreshCycle);
    settings.setValue("plot/hidelowsat", hideLowSatellites);
    settings.setValue("plot/elmaskp", elevationMaskP);
    settings.setValue("plot/exsats", excludedSatellites);
    settings.setValue("plot/rtbuffsize", rtBufferSize);
    settings.setValue("plot/timesyncout", timeSyncOut);
    settings.setValue("plot/timesyncport", timeSyncPort);
    settings.setValue("plot/rtstream1", rtStream[0]);
    settings.setValue("plot/rtstream2", rtStream[1]);
    settings.setValue("plot/rtformat1", rtFormat[0]);
    settings.setValue("plot/rtformat2", rtFormat[1]);
    settings.setValue("plot/rttimeform", rtTimeFormat);
    settings.setValue("plot/rtdegform", rtDegFormat);
    settings.setValue("plot/rtfieldsep", rtFieldSeperator);
    settings.setValue("plot/rttimeouttime", rtTimeoutTime);
    settings.setValue("plot/rtreconntime", rtReconnectTime);

    settings.setValue("plot/mcolor0", mColor[0][0]);
    settings.setValue("plot/mcolor1", mColor[0][1]);
    settings.setValue("plot/mcolor2", mColor[0][2]);
    settings.setValue("plot/mcolor3", mColor[0][3]);
    settings.setValue("plot/mcolor4", mColor[0][4]);
    settings.setValue("plot/mcolor5", mColor[0][5]);
    settings.setValue("plot/mcolor6", mColor[0][6]);
    settings.setValue("plot/mcolor7", mColor[0][7]);
    settings.setValue("plot/mcolor8", mColor[0][0]);
    settings.setValue("plot/mcolor9", mColor[1][1]);
    settings.setValue("plot/mcolor10", mColor[1][2]);
    settings.setValue("plot/mcolor11", mColor[1][3]);
    settings.setValue("plot/mcolor12", mColor[1][4]);
    settings.setValue("plot/mcolor13", mColor[1][5]);
    settings.setValue("plot/mcolor14", mColor[1][6]);
    settings.setValue("plot/mcolor15", mColor[1][7]);
    settings.setValue("plot/mapcolor1", mapColor[0]);
    settings.setValue("plot/mapcolor2", mapColor[1]);
    settings.setValue("plot/mapcolor3", mapColor[2]);
    settings.setValue("plot/mapcolor4", mapColor[3]);
    settings.setValue("plot/mapcolor5", mapColor[4]);
    settings.setValue("plot/mapcolor6", mapColor[5]);
    settings.setValue("plot/mapcolor7", mapColor[6]);
    settings.setValue("plot/mapcolor8", mapColor[7]);
    settings.setValue("plot/mapcolor9", mapColor[8]);
    settings.setValue("plot/mapcolor10", mapColor[9]);
    settings.setValue("plot/mapcolor11", mapColor[10]);
    settings.setValue("plot/mapcolor12", mapColor[11]);
    settings.setValue("plot/mapcolorf1", mapColorF[0]);
    settings.setValue("plot/mapcolorf2", mapColorF[1]);
    settings.setValue("plot/mapcolorf3", mapColorF[2]);
    settings.setValue("plot/mapcolorf4", mapColorF[3]);
    settings.setValue("plot/mapcolorf5", mapColorF[4]);
    settings.setValue("plot/mapcolorf6", mapColorF[5]);
    settings.setValue("plot/mapcolorf7", mapColorF[6]);
    settings.setValue("plot/mapcolorf8", mapColorF[7]);
    settings.setValue("plot/mapcolorf9", mapColorF[8]);
    settings.setValue("plot/mapcolorf10", mapColorF[9]);
    settings.setValue("plot/mapcolorf11", mapColorF[10]);
    settings.setValue("plot/mapcolorf12", mapColorF[11]);
    settings.setValue("plot/color1", cColor[0]);
    settings.setValue("plot/color2", cColor[1]);
    settings.setValue("plot/color3", cColor[2]);
    settings.setValue("plot/color4", cColor[3]);

    settings.setValue("plot/staposfile", plotOptDialog->refDialog->stationPositionFile);

    settings.setValue("plot/elmask", elevationMask);
    settings.setValue("plot/maxdop", maxDop);
    settings.setValue("plot/maxmp", maxMP);
    settings.setValue("plot/yrange", yRange);
    settings.setValue("plot/orgin", origin);
    settings.setValue("plot/rcvpos", receiverPosition);
    settings.setValue("plot/oopos1", ooPosition[0]);
    settings.setValue("plot/oopos2", ooPosition[1]);
    settings.setValue("plot/oopos3", ooPosition[2]);
    settings.setValue("plot/shapefile", shapeFile);
    settings.setValue("plot/tlefile", tleFile);
    settings.setValue("plot/tlesatfile", tleSatelliteFile);

    settings.setValue("plot/fontname", font.family());
    settings.setValue("plot/fontsize", font.pointSize());

    settings.setValue("plot/rnxopts", rinexOptions);
    settings.setValue("mapview/apikey", apiKey);
    settings.setValue("mapview/mapapi", mapApi);
     for (int i=0;i<6;i++) for (int j=0;j<3;j++) {
         settings.setValue(QString("mapview/mapstrs_%1_%2").arg(i).arg(j),mapStreams[i][j]);
     }

    for (int i = 0; i < 2; i++) {
        settings.setValue(QString("str/strcmd1_%1").arg(i), streamCommands  [0][i]);
        settings.setValue(QString("str/strcmd2_%1").arg(i), streamCommands  [1][i]);
        settings.setValue(QString("str/strcmdena1_%1").arg(i), streamCommandEnabled[0][i]);
        settings.setValue(QString("str/strcmdena2_%1").arg(i), streamCommandEnabled[1][i]);
    }
    for (int i = 0; i < 3; i++) {
        settings.setValue(QString("str/strpath1_%1").arg(i), streamPaths[0][i]);
        settings.setValue(QString("str/strpath2_%1").arg(i), streamPaths[1][i]);
    }
    for (int i = 0; i < 10; i++) {
        settings.setValue(QString("str/strhistry_%1").arg(i), streamHistory [i]);
    }

    settings.setValue("viewer/color1", TextViewer::colorText);
    settings.setValue("viewer/color2", TextViewer::colorBackground);
    settings.setValue("viewer/fontname", TextViewer::font.family());
    settings.setValue("viewer/fontsize", TextViewer::font.pointSize());

    settings.setValue("solbrows/dir", fileSelDialog->directory);
    settings.setValue("solbrows/split1", BrowseSplitter->saveState());
}

//---------------------------------------------------------------------------
void Plot::driveSelectChanged()
{
    tVdirectorySelector->setVisible(false);

    tVdirectorySelector->setRootIndex(dirModel->index(cBDriveSelect->currentText()));
    lblDirectorySelected->setText(cBDriveSelect->currentText());
}
//---------------------------------------------------------------------------
void Plot::btnDirectorySelectorClicked()
{
#ifdef FLOATING_DIRSELECTOR
    QPoint pos = Panel5->mapToGlobal(BtnDirSel->pos());
    pos.rx() += BtnDirSel->width() - DirSelector->width();
    pos.ry() += BtnDirSel->height();

    DirSelector->move(pos);
#endif
    tVdirectorySelector->setVisible(!tVdirectorySelector->isVisible());
}
//---------------------------------------------------------------------------
void Plot::directorySelectionChanged(QModelIndex index)
{
    tVdirectorySelector->expand(index);

    directory = dirModel->filePath(index);
    lblDirectorySelected->setText(directory);
    fileModel->setRootPath(directory);
    lVFileList->setRootIndex(fileModel->index(directory));
}
//---------------------------------------------------------------------------
void Plot::directorySelectionSelected(QModelIndex)
{
    tVdirectorySelector->setVisible(false);
}
//---------------------------------------------------------------------------
void Plot::fileListClicked(QModelIndex index)
{
    QStringList file;

    file.append(fileModel->filePath(index));
    plot->readSolution(file, 0);

    tVdirectorySelector->setVisible(false);
}
//---------------------------------------------------------------------------
void Plot::filterClicked()
{
    QString filter = cBFilter->currentText();

    // only keep data between brackets
    filter = filter.mid(filter.indexOf("(") + 1);
    filter.remove(")");

    fileModel->setNameFilters(filter.split(" "));
    tVdirectorySelector->setVisible(false);
}
//---------------------------------------------------------------------------
void Plot::btnFrequencyClicked()
{
    freqDialog->exec();
}
//---------------------------------------------------------------------------
