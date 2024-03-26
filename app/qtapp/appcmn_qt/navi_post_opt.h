//---------------------------------------------------------------------------
#ifndef navioptH
#define navioptH
//---------------------------------------------------------------------------
#include <QDialog>
#include "rtklib.h"

namespace Ui {
class OptDialog;
}

class TextViewer;
class FreqDialog;
class QLineEdit;
class QSettings;

//---------------------------------------------------------------------------
class OptDialog : public QDialog
{
    Q_OBJECT

public:
    enum OptionsType {NaviOptions = 0, PostOptions = 1};

    explicit OptDialog(QWidget* parent, int opts);

    void loadOptions(QSettings &);
    void saveOptions(QSettings &);

    opt_t *appOptions;  // additional application specific options to load and save

    prcopt_t processingOptions;
    solopt_t solutionOptions;
    filopt_t fileOptions;

    // RTKNavi options
    int serverCycle, serverBufferSize, solutionBufferSize, navSelect, savedSolutions;
    int nmeaCycle, timeoutTime, reconnectTime;
    int monitorPort, fileSwapMargin, panelStacking;
    QString proxyAddress;
    QFont panelFont, positionFont;
    QColor panelFontColor, positionFontColor;

    // RTKPost options
    QString roverList, baseList;

protected:
    void showEvent(QShowEvent*);
    QString excludedSatellitesString(prcopt_t *prcopt);
    bool fillExcludedSatellites(prcopt_t *prcopt, const QString &excludedSatellites);

    char proxyaddr[1024];  // proxy address stores in naviopts
    opt_t *naviopts;
    snrmask_t snrmask;
    int current_roverPositionType, current_referencePositionType;

    TextViewer *textViewer;
    FreqDialog * freqDialog;

protected slots:
    void saveClose();
    void selectAntennaPcvFile();
    void viewAntennaPcvFile();
    void loadSettings();
    void saveSettings();
    void selectReferencePosition();
    void selectRoverPosition();
    void viewStationPositionFile();
    void selectStationPositionFile();
    void referencePositionTypeChanged(int);
    void roverPositionTypeChanged(int);
    void getPosition(int type, QLineEdit **edit, double *pos);
    void setPosition(int type, QLineEdit **edit, double *pos);
    void selectPanelFont();
    void selectSolutionFont();
    void selectGeoidDataFile();
    void viewSatellitePcvFile();
    void selectSatellitePcvFile();
    void selectLocalDirectory();
    void selectIonosphereFile();
    void selectDCBFile();
    void viewDCBFile();
    void selectEOPFile();
    void viewEOPFile();
    void selectBLQFile();
    void viewBLQFile();
    void showSnrMaskDialog();

private:
    void load(const QString &file);
    void save(const QString &file);
    void readAntennaList();
    void updateEnable();
    void showFrequenciesDialog();
    void showKeyDialog();
    int options;
    Ui::OptDialog *ui;

};
//---------------------------------------------------------------------------
#endif
