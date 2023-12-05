//---------------------------------------------------------------------------
#ifndef plotmainH
#define plotmainH
//---------------------------------------------------------------------------
#include <QTimer>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QFileSystemModel>
#include <QTreeView>

#include "graph.h"
#include "console.h"
#define INHIBIT_RTK_LOCK_MACROS
#include "rtklib.h"

#include "ui_plotmain.h"

#define MAXNFILE    256                 // max number of solution files
#define MAXSTRBUFF  1024                // max length of stream buffer
#define MAXWAYPNT   4096                // max number of waypoints
#define MAXMAPPATH  4096                // max number of map paths
#define MAXMAPLAYER 12                  // max number of map layers

#define PRGNAME     "RTKPLOT-QT"           // program name

const QChar degreeChar(0260);           // character code of degree (UTF-8)
const QChar up2Char(0262);              // character code of ^2     (UTF-8)

#define DEFTSPAN    600.0               // default time span (s)
#define INTARROW    60.0                // direction arrow interval (s)
#define MAXTDIFF    60.0                // max differential time (s)
#define DOPLIM      30.0                // dop view limit
#define TTOL        DTTOL               // time-differnce tolerance (s)
#define TBRK        300.0               // time to recognize break (s)
#define THRESLIP    0.1                 // slip threshold of LG-jump (m)
#define SIZE_COMP   45                  // compass size (pixels)
#define SIZE_VELC   45                  // velocity circle size (pixels)
#define MIN_RANGE_GE 10.0               // min range for GE view

#define PLOT_TRK    0                   // plot-type: track-plot
#define PLOT_SOLP   1                   // plot-type: position-plot
#define PLOT_SOLV   2                   // plot-type: velocity-plot
#define PLOT_SOLA   3                   // plot-type: accel-plot
#define PLOT_NSAT   4                   // plot-type: number-of-satellite-plot
#define PLOT_RES    5                   // plot-type: residual-plot
#define PLOT_RESE   6                   // plot-type: residual-elevation plot
#define PLOT_OBS    7                   // plot-type: observation-data-plot
#define PLOT_SKY    8                   // plot-type: sky-plot
#define PLOT_DOP    9                   // plot-type: dop-plot
#define PLOT_SNR    10                  // plot-type: snr/mp-plot
#define PLOT_SNRE   11                  // plot-type: snr/mp-el-plot
#define PLOT_MPS    12                  // plot-type: mp-skyplot

#define ORG_STARTPOS 0                  // plot-origin: start position
#define ORG_ENDPOS  1                   // plot-origin: end position
#define ORG_AVEPOS  2                   // plot-origin: average position
#define ORG_FITPOS  3                   // plot-origin: linear-fit position
#define ORG_REFPOS  4                   // plot-origin: reference position
#define ORG_LLHPOS  5                   // plot-origin: lat/lon/hgt position
#define ORG_AUTOPOS 6                   // plot-origin: auto-input position
#define ORG_IMGPOS  7                   // plot-origin: image-center position
#define ORG_MAPPOS  8                   // plot-origin: map center position
#define ORG_PNTPOS  9                   // plot-origin: way-point position

#define TRACEFILE   "rtkplot.trace"     // trace file
#define QCTMPFILE   "rtkplot_qc.temp"   // temporary file for qc
#define QCERRFILE   "rtkplot_qc.err"    // error file for qc

#define SQR(x)      ((x)*(x))
#define SQRT(x)     ((x)<0.0||(x)!=(x)?0.0:sqrt(x))
#define MAX(x,y)    ((x)>(y)?(x):(y))
#define MIN(x,y)    ((x)<(y)?(x):(y))

extern const QString PTypes[];

class FreqDialog;
class MapOptDialog;
class MapView;
class SpanDialog;
class ConnectDialog;
class SkyImgDialog;
class PlotOptDialog;
class AboutDialog;
class PntDialog;
class FileSelDialog;
class TextViewer;
class VecMapDialog;

class QEvent;
class QMouseEvent;
class QFocusEvent;
class QKeyEvent;
class QResizeEvent;
class QShowEvent;
class QWheelEvent;
class QPaintEVent;

// time-position class ------------------------------------------------------
class TIMEPOS
{
private:
    int nmax_;
    TIMEPOS(TIMEPOS &){}

public:
    int n;
    gtime_t *t;
    double *x, *y, *z, *xs, *ys, *zs, *xys;
    int *q;
    TIMEPOS(int nmax, int sflg);
    ~TIMEPOS();
    TIMEPOS *tdiff(void);
    TIMEPOS *diff(const TIMEPOS *pos2, int qflag);
};

// rtkplot class ------------------------------------------------------------
class Plot : public QMainWindow, public Ui::Plot
{
    Q_OBJECT

protected:
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);
    void closeEvent(QCloseEvent*);
    void keyPressEvent(QKeyEvent *);
    void showEvent(QShowEvent*);
    void resizeEvent(QResizeEvent *);
    void leaveEvent(QEvent*);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void paintEvent(QPaintEvent *event);

    void mouseMove(QMouseEvent*);
public slots:
    void menuOpenSolution1Clicked();
    void menuOpenSolution2Clicked();
    void menuOpenMapImageClicked();
    void menuOpenShapeClicked();
    void menuOpenObservationClicked();
    void menuOpenNavigatioClicked();
    void menuOpenElevationMaskClicked();
    void menuConnectClicked();
    void menuDisconnectClicked();
    void menuPortClicked();
    void menuReloadClicked();
    void menuClearClicked();
    void menuQuitClicked();

    void menuTimeClicked();
    void menuMapImageClicked();
    void menuWaypointClicked();
    void menuSrcSolutionClicked();
    void menuSrcObservationClicked();
    void menuCopyClicked();
    void menuOptionsClicked();

    void menuToolBarClicked();
    void menuStatusBarClicked();
    void menuMonitor1Clicked();
    void menuMonitor2Clicked();
    void menuMapViewClicked();
    void menuCenterOriginClicked();
    void menuFitHorizontalClicked();
    void menuFitVerticalClicked();
    void menuShowTrackClicked();
    void menuFixHorizontalClicked();
    void menuFixVerticalClicked();
    void menuShowMapClicked();
    void menuShowImageClicked();
    void menuShowGridClicked();
    void menuAnimationStartClicker();
    void menuAnimationStopClicker();
    void menuAboutClicked();

    void btnConnectClicked();
    void btnSolution1Clicked();
    void btnSolution2Clicked();
    void btnSolution12Clicked();
    void btnSolution1DblClicked();
    void btnSolution2DblClicked();
    void btnOn1Clicked();
    void btnOn2Clicked();
    void btnOn3Clicked();
    void btnRangeListClicked();
    void btnAnimateClicked();
    void btnFrequencyClicked();

    void plotTypeSChanged();
    void qFlagChanged();
    void observationTypeChanged();
    void dopTypeChanged();
    void satelliteListChanged();
    void timeScrollbarChanged();
    void rangeListClicked();

    void timerTimer();

    void menuSaveDopClicked();
    void menuSaveImageClicked();
    void menuVisibilityAnalysisClicked();
    void menuFixCenterClicked();
    void menuSaveSnrMpClicked();
    void menuOpenSkyImageClicked();
    void menuSkyImgClicked();
    void menuShowSkyplotClicked();
    void menuPlotMapViewClicked();
    void menuMaxClicked();
    void menuSaveElevationMaskClicked();
    void menuMapLayerClicked();
    void btnMessage2Clicked();
    void menuOpenWaypointClicked();
    void menuSaveWaypointClicked();
    void menuBrowseClicked();

    void fileListClicked(QModelIndex);
    void directorySelectionChanged(QModelIndex);
    void directorySelectionSelected(QModelIndex);
    void driveSelectChanged();
    void filterClicked();
    void btnDirectorySelectorClicked();

private:
    bool eventFilter(QObject *obj, QEvent *event);
    QTreeView *tVdirectorySelector;
    QFileSystemModel *fileModel, *dirModel;
    QString directory;

    QPixmap buffer;
    QImage mapImage;
    QImage skyImageI;
    Graph *graphT;
    Graph *graphG[3];
    Graph *graphR;
    Graph *graphS;
    Graph *graphE[2];
    Console *console1, *console2;
    QStringList openFiles;
    QStringList solutionFiles[2];
    QStringList observationFiles;
    QStringList navigationFiles;

    stream_t stream[2];
    stream_t streamTimeSync;
    solbuf_t solutionData[2];
    solstatbuf_t solutionStat[2];
    int solutionIndex[2];
    obs_t observation;
    nav_t navigation;
    sta_t station;
    double *azimuth, *elevtion, *multipath[NFREQ+NEXOBS];
    char streamBuffer[1024];
    int nStreamBuffer;
    QTimer timer;
    QElapsedTimer updateTimer;
    
    gtime_t oEpoch;
    int formWidth, formHeight;
    int connectState, openRaw;
    int nObservation, *indexObservation, observationIndex;
    int week;
    int flush, plotType;
    int nSolutionF1, nSolutionF2, nObservationF, nNavigationF;
    int satelliteMask[MAXSAT], satelliteSelection[MAXSAT];
    int simObservation;
    
    int Drag, Xn, Yn;
    double X0, Y0, Xc, Yc, Xs, Ys, Xcent, Xcent0;
    uint32_t mouseDownTick;
    
    int GEState, GEDataState[2];
    double GEHeading;

    void readSolutionStat(const QStringList &files, int sel);
    int readObservationRinex(const QStringList &files, obs_t *obs, nav_t *nav, sta_t *sta);
    void readMapTag(const QString &file);
    void readShapeFile(const QStringList &file);
    void generateVisibilityData(void);
    void saveDop(const QString &file);
    void saveSnrMp(const QString &file);
    void saveElevationMask(const QString &file);
    void connectStream(void);
    void disconnectStream(void);
    void connectPath(const QString &path, int ch);
    int checkObservation(const QString &file);
    void updateObservation(int nobs);
    void updateMp(void);
    void clearObservation(void);
    void clearSolution(void);
    void clear(void);
    void refresh(void);
    void reload(void);
    void readWaitStart(void);
    void readWaitEnd(void);
    
    void updateDisplay(void);
    void updateType(int type);
    void updatePlotType(void);
    void updateSatelliteList(void);
    void updateObservationType(void);
    void updateSize(void);
    void updateColor(void);
    void updateTime(void);
    void updateOrigin(void);
    void updateSatelliteMask(void);
    void updateSatelliteSelection(void);
    void updateInfo(void);
    void updateTimeSolution(void);
    void updateTimeObservation(void);
    void updateInfoSolution(void);
    void updateInfoObservation(void);
    void updatePoint(int x, int y);
    void updateEnable(void);
    void fitTime(void);
    void setRange(int all, double range);
    void fitRange(int all);
    
    void setCenterX(double c);
    void setScaleX(double s);
    void mouseDownTrack(int X, int Y);
    void mouseDownSolution(int X, int Y);
    void mouseDownObservation(int X, int Y);
    void mouseMoveTrack(int X, int Y, double dx, double dy, double dxs, double dys);
    void mouseMoveSolution(int X, int Y, double dx, double dy, double dxs, double dys);
    void mouseMoveObservation(int X, int Y, double dx, double dy, double dxs, double dys);

    void drawTrack(QPainter &g,int level);
    void drawTrackImage(QPainter &g,int level);
    void drawTrackMap(QPainter &g,int level);
    void drawTrackPath(QPainter &g,int level);
    void drawTrackPoint(QPainter &g,const TIMEPOS *pos, int level, int style);
    void drawTrackPosition(QPainter &g,const double *rr, int type, int siz, QColor color, const QString &label);
    void drawTrackStat(QPainter &g,const TIMEPOS *pos, const QString &header, int p);
    void drawTrackError(QPainter &g,const TIMEPOS *pos, int style);
    void drawTrackArrow(QPainter &g,const TIMEPOS *pos);
    void drawTrackVelocity(QPainter &g,const TIMEPOS *vel);
    void drawLabel(Graph *,QPainter &g, const QPoint &p, const QString &label, int ha, int va);
    void drawMark(Graph *,QPainter &g, const QPoint &p, int mark, const QColor &color, int size, int rot);
    void drawSolution(QPainter &g,int level, int type);
    void DdrawSolutionPoint(QPainter &g,const TIMEPOS *pos, int level, int style);
    void drawSolutionStat(QPainter &g,const TIMEPOS *pos, const QString &unit, int p);
    void drawNsat(QPainter &g,int level);
    void drawResidual(QPainter &g,int level);
    void drawResidualE(QPainter &g,int level);
    void drawPolyS(Graph *,QPainter &c, double *x, double *y, int n,
                           const QColor &color, int style);
    
    void drawObservation(QPainter &g,int level);
    void drawObservationSlip(QPainter &g,double *yp);
    void drawObservationEphemeris(QPainter &g,double *yp);
    void drawSkyImage(QPainter &g,int level);
    void drawSky(QPainter &g,int level);
    void drawDop(QPainter &g,int level);
    void drawDopStat(QPainter &g,double *dop, int *ns, int n);
    void drawSnr(QPainter &g,int level);
    void drawSnrE(QPainter &g,int level);
    void drawMpS(QPainter &g,int level);
    
    TIMEPOS *solutionToPosition(solbuf_t *sol, int index, int qflag, int type);
    TIMEPOS *solutionToNsat(solbuf_t *sol, int index, int qflag);
    
    void positionToXyz(gtime_t time, const double *rr, int type, double *xyz);
    void covarianceToXyz(const double *rr, const float *qr, int type,
                                    double *xyzs);
    void calcStats(const double *x, int n, double ref, double &ave,
                                    double &std, double &rms);
    int fitPosition(gtime_t *time, double *opos, double *ovel);
    
    QString latLonString(const double *pos, int ndec);
    QColor observationColor(const obsd_t *obs, double az, double el);
    QColor sysColor(int sat);
    QColor snrColor(double snr);
    QColor mpColor(double mp);
    void readStationPosition(const QString &file, const QString &sta, double *rr);
    int searchPosition(int x, int y);
    void timeSpan(gtime_t *ts, gtime_t *te, double *tint);
    double timePosition(gtime_t time);
    void timeStream(gtime_t time, int n, int tsys, QString &str);
    int execCmd(const QString &cmd, const QStringList &opt);
    void showMessage(const QString &msg);
    void showLegend (QString *msgs);
    void loadOptions();
    void saveOption();
    
    FreqDialog * freqDialog;
    MapOptDialog *mapOptDialog;
    MapView *mapView;
    SpanDialog *spanDialog;
    ConnectDialog *connectDialog;
    SkyImgDialog *skyImgDialog;
    PlotOptDialog *plotOptDialog;
    AboutDialog *aboutDialog;
    PntDialog *pntDialog;
    FileSelDialog *fileSelDialog;
    TextViewer *viewer;
    VecMapDialog *vecMapDialog;

public:
    QImage skyImageR;
    QString iniFile;
    QString mapImageFile;
    QString skyImageFile;
    QString rinexOptions;
    tle_t tleData;
    QFont font;
    gis_t gis;

    // connection settings
    int rtStream[2];
    QString rtPath1, rtPath2;
    QString streamPaths[2][3];
    QString streamCommands[2][2];
    int streamCommandEnabled[2][2];
    int rtFormat[2];
    int rtConnectionType;
    int rtTimeFormat;
    int rtDegFormat;
    QString rtFieldSeperator;
    int rtTimeoutTime;
    int rtReconnectTime;
    double elevationMaskData[361];
    
    // time options
    int timeEnabled[3];
    gtime_t timeStart;
    gtime_t timeEnd;
    double timeInterval;
    
    // map options
    int mapSize[2], mapScaleEqual;
    double mapScaleX, mapScaleY;
    double mapLatitude, mapLongitude;
    int pointType;
    
    // sky image options
    int skySize[2], skyDestCorrection, skyElevationMask, skyRes, skyFlip, skyBinarize;
    double skyCenter[2], skyScale, skyScaleR, skyFOV[3], skyDest[10];
    double skyBinThres1, skyBinThres2;
    
    // plot options
    int timeLabel;
    int latLonFormat;
    int showStats;
    int showSlip;
    int showHalfC;
    int showEphemeris;
    double elevationMask;
    int elevationMaskP;
    int hideLowSatellites;
    double maxDop, maxMP;
    int navSys;
    QString excludedSatellites;
    int showError;
    int showArrow;
    int showGridLabel;
    int showLabel;
    int showCompass;
    int showScale;
    int autoScale;
    double yRange;
    int rtBufferSize;
    int timeSyncOut;
    int timeSyncPort;
    int origin;
    int receiverPosition;
    double ooPosition[3];
    QColor mColor[2][8]; // {{mark1 0-7},{mark2 0-7}}
    QColor cColor[4];    // {background,grid,text,line}
    QColor mapColor[MAXMAPLAYER]; // mapcolors line
    QColor mapColorF[MAXMAPLAYER]; // mapcolors fill
    int plotStyle;
    int markSize;
    int animationCycle;
    int refreshCycle;
    int traceLevel;
    QString fontName;
    int fontSize;
    QString shapeFile;
    QString tleFile;
    QString tleSatelliteFile;
    // map view options
    int mapApi;
    QString mapStreams[6][3],apiKey;
    
    QString title;
    QString pointName[MAXWAYPNT];
    double pointPosition[MAXWAYPNT][3];
    int nWayPoint, selectedWayPoint;
    int oPositionType;
    double oPosition[3], oVelocity[3];
    
    QString streamHistory [10];
    
    void readSolution(const QStringList &files, int sel);
    void readObservation(const QStringList &files);
    void readNavigation(const QStringList &files);
    void readMapData(const QString &file);
    void readSkyData(const QString &file);
    void readGpxFile(const QString &file);
    void readPositionFile(const QString &file);
    void readWaypoint(const QString &file);
    void saveGpxFile(const QString &file);
    void savePositionFile(const QString &file);
    void saveWaypoint(const QString &file);
    void readSkyTag(const QString &file);
    void updateSky(void);
    void updatePlot (void);
    void generateObservationSlip(int *LLI);
    void readElevationMaskData(const QString &file);
    int getCurrentPosition(double *rr);
    int getCenterPosition(double *rr);
    void setTrackCenter(double lat, double lon);
    void refresh_MapView(void);

    explicit Plot(QWidget* parent=NULL);
    ~Plot();
};

//---------------------------------------------------------------------------
#endif
