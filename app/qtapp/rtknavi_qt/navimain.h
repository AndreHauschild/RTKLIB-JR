//---------------------------------------------------------------------------
#ifndef navimainH
#define navimainH
//---------------------------------------------------------------------------
#include <QDialog>
#include <QColor>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QMenu>


#include "rtklib.h"
#include "ui_navimain.h"

#define MAXSCALE	18
#define MAXMAPPNT	10

class AboutDialog;
class OptDialog;
class InputStrDialog;
class OutputStrDialog;
class LogStrDialog;
class MonitorDialog;

//---------------------------------------------------------------------------
class MainWindow : public QDialog, private Ui::MainForm
{
        Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void updateTimerTriggered();

    void startClicked();
    void stopClicked();
    void showRtkPlot();
    void btnOptionsClicked();
    void exitClicked();

    void btnTimeSystemClicked();
    void btnInputStreamClicked();
    void btnOutputStreamClicked();
    void btnLogStreamClicked();
    void btnSolutionTypeClicked();
    void btnPlotType1Clicked();

    void showMonitorDialog();
    void btnSaveClicked();
    void btnAboutClicked();
    void btnTaskTrayClicked();

    void menuExpandClicked();
    void sBSolutionChanged();

    void trayIconClicked(QSystemTrayIcon::ActivationReason);
    void btnFrequencyType1Clicked();
    void btnPanelClicked();
    void btnPlotType2Clicked();
    void btnFrequencyType2Clicked();
    void btnPlotType3Clicked();
    void btnFrequencyType3Clicked();
    void btnPlotType4Clicked();
    void btnFrequencyType4Clicked();
    void btnExpand1Clicked();
    void btnShrink1Clicked();
    void btnExpand2Clicked();
    void btnShrink2Clicked();
    void btnExpand3Clicked();
    void btnShrink3Clicked();
    void btnExpand4Clicked();
    void btnShrink4Clicked();
    void btnMarkClicked();

protected:
    void showEvent(QShowEvent*);
    void closeEvent(QCloseEvent *);

private:
    AboutDialog *aboutDialog;
    InputStrDialog *inputStrDialog;
    OutputStrDialog *outputStrDialog;
    LogStrDialog *logStrDialog;
    MonitorDialog *monitor;
    QSystemTrayIcon *systemTray;
    QMenu *trayMenu;
    QAction *menuStartAction, *menuStopAction, *menuExitAction;

    void updateLog (int stat, gtime_t time, double *rr, float *qr,
                     double *rb, int ns, double age, double ratio);
    void serverStart();
    void serverStop();
    void updatePanel();
    void updateTimeSystem();
    void updateSolutionType();
    void updateFont();
    void updateTime();
    void updatePosition();
    void updateStream();
    void drawSolutionPlot(QLabel *plot, int type, int freq);
    void updatePlot();
    void updateEnable();
    void ChangePlot(void);
    int confirmOverwrite(const QString &path);

    void drawSnr(QPainter &c, int w, int h, int x0, int y0, int index, int freq);
    void drawSatellites(QPainter &c, int w, int h, int x0, int y0, int index, int freq);
    void drawBaseline(QPainter &c, int id, int w, int h);
    void drawTrack(QPainter &c, int id, QPaintDevice * plot);
    void drawSky(QPainter &c, int w, int h, int x0, int y0);
    void drawText(QPainter &c, int x, int y, const QString &s,
                   const QColor &color, int ha, int va);
    void drawArrow(QPainter &c, int x, int y, int siz,
                                  int ang, const QColor &color);
    void openMonitorPort(int port);
    void initSolutionBuffer(void);
    void saveLogs(void);
    void loadNavigation(nav_t *nav);
    void saveNavigation(nav_t *nav);
    void loadOptions(void);
    void saveOptions(void);
    void setTrayIcon(int index);
    int execCommand(const QString &cmd, const QStringList &opt, int show);
    void btnFrequencyTypeChanged(int i);
    QColor snrColor(int snr);
public:
    OptDialog *optDialog;
    QString iniFile;

    int timerCycle,timerInactive;
    int panelStacking, panelMode;
    int serverCycle, serverBufferSize, scale, solutionBufferSize, NavSelect, savedSolutions;

    int NmeaReq, nmeaCycle, inputTimeTag, inputTime64Bit;
    int outputTimeTag, outputAppend, logTimeTag,logAppend;
    int timeoutTime, reconnectionTime, sbasCorrection, dgpsCorrection, tideCorrection, fileSwapMargin;
    int streamType[MAXSTRRTK], streamEnabled[MAXSTRRTK], inputFormat[MAXSTRRTK];
    int commandEnabled[3][3], commandEnableTcp[3][3];
    int timeSystem, solutionType;
    int plotType[4],frequencyType[4];
    int trackType[4];
    int trackScale[4];
    int baselineMode[4];
    int monitorPort, monitorPortOpen, autoRun;

    int solutionsCurrent, solutionsStart, solutionsEnd, numSatellites[2], solutionCurrentStatus;
    int satellites[2][MAXSAT], satellitesSNR[2][MAXSAT][NFREQ], validSatellites[2][MAXSAT];
    double satellitesAzimuth[2][MAXSAT], satellitesElevation[2][MAXSAT];
    gtime_t *timeStamps;
    int *solutionStatus, *numValidSatellites;
    double *solutionRover, *solutionReference, *solutionQr, *velocityRover, *ages, *ratioAR;
    double trackOrigin[3], maxBaseLine;

    QString paths[MAXSTRRTK][4], commands[3][3], commandsTcp[3][3];
    QString inputTimeStart, inputTimeSpeed, excludedSatellites;
    QString receiverOptions[3], proxyAddress;
    QString outputSwapInterval, logSwapInterval, resetCommand;

    prcopt_t processingOptions;
    solopt_t solutionOptions;
    
    QFont panelFont, positionFont;
    QColor panelFontColor, positionFontColor;

    int debugTraceLevel, debugStatusLevel, outputGeoidF, baselineEnabled;
    int roverPositionTypeF, referencePositionTypeF, roverAntennaPcvF, referenceAntennaPcvF;
    QString roverAntennaF, referenceAntennaF, satellitePcvFileF, antennaPcvFileF;
    double roverAntennaDelta[3], referenceAntennaDelta[3], roverPosition[3], referencePosition[3], nmeaPosition[3];
    double baseline[2];

    QString history[10];

    QTimer updateTimer;

    QString geoidDataFileF, stationPositionFileF, dcbFile, eopFileF;
    QString localDirectory, pointName[MAXMAPPNT];

    double pointPosition[MAXMAPPNT][3];
    int nMapPoint;

    QString markerName, markerComment;
};

extern MainWindow *mainForm;
//---------------------------------------------------------------------------
#endif
