//---------------------------------------------------------------------------
#ifndef navioptH
#define navioptH
//---------------------------------------------------------------------------
#include <QDialog>
#include "ui_naviopt.h"
#include "rtklib.h"

class TextViewer;
class FreqDialog;
//---------------------------------------------------------------------------
class OptDialog : public QDialog, private Ui::OptDialog
{
    Q_OBJECT
protected:
    void showEvent(QShowEvent*);

public slots:
    void btnOkClicked();
    void btnAntennaPcvFileClicked();
    void btnDCBFileClicked();
    void btnAntennaPcvViewClicked();
    void btnLoadClicked();
    void btnSaveClicked();
    void btnReferencePositionClicked();
    void btnRoverPositionClicked();
    void btnStationPositionViewClicked();
    void btnStationPositionFileClicked();
    void referencePositionTypePChanged(int);
    void roverPositionTypePChanged(int);
    void getPosition(int type, QLineEdit **edit, double *pos);
    void setPosition(int type, QLineEdit **edit, double *pos);
    void btnFontPanelClicked();
    void btnFontSolutionClicked();
    void btnGeoidDataFileClicked();
    void btnSatellitePcvViewClicked();
    void btnSatellitePcvFileClicked();
    void btnLocalDirectoryClicked();
    void btnEOPFileClicked();
    void btnEOPViewClicked();
    void btnSnrMaskClicked();

private:
    void getOption(void);
    void setOption(void);
    void loadOption(const QString &file);
    void saveOption(const QString &file);
    void readAntennaList(void);
    void updateEnable(void);

public:
    prcopt_t processOptions;
    solopt_t solutionOption;
    QFont panelFont, positionFont;
    TextViewer *textViewer;
    FreqDialog * freqDialog;

    int serverCycle, serverBufferSize, solutionBufferSize, navSelect, savedSolution;
    int nmeaRequest, nmeaCycle, timeoutTime, reconnectTime, dgpsCorrection, sbasCorrection;
    int debugTraceF, debugStatusF;
    int roverPositionTypeF, referencePositionTypeF, roverAntennaPcvF, referenceAntennaPcvF, baselineC;
    int monitorPort, fileSwapMargin, panelStack;

    QString excludedSatellites, localDirectory;
    QString roverAntennaF, referenceAntennaF, satellitePcvFileF, antennaPcvFileF, stationPositionFileF;
    QString geoidDataFileF, dcbFileF, eopFileF, tleFileF, tleSatFileF;
    QString proxyAddr;

    double roverAntennaDelta[3], referenceAntennaDelta[3], roverPosition[3], referencePosition[3];
    double baseline[2], nmeaInterval[2];

    explicit OptDialog(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
