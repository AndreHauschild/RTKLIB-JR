//---------------------------------------------------------------------------
#ifndef mondlgH
#define mondlgH
//---------------------------------------------------------------------------
#include <QStringList>
#include <QDialog>
#include <QTimer>

#include "rtklib.h"

#include "ui_mondlg.h"

//---------------------------------------------------------------------------
class MonitorDialog : public QDialog, private Ui::MonitorDialog
{
    Q_OBJECT
protected:
    void showEvent(QShowEvent*);
    void closeEvent(QCloseEvent*);

public slots:
    void btnClearClicked();
    void btnDownClicked();
    void displayTypeChanged();
    void updateTimerTriggered();
    void showBuffers();
    void observationModeChanged();
    void consoleFormatChanged();
    void inputStreamChanged();
    void solutionStreamChanged();

private:
    int typeF, consoleFormat, inputStream, solutionStream, fontScale, observationMode;
    QStringList consoleBuffer;
    QStringList header;
	rtcm_t rtcm;
	raw_t raw;
    QTimer updateTimer;
	
    void clearTable();
    void setRtk();
    void setSat();
    void setEstimates();
    void setCovariance();
    void setObservations();
    void setNavigation();
    void setGlonassNavigations();
    void setStream();
    void setSbsMessages();
    void setSbsLong();
    void setSbsIono();
    void setSbsFast();
    void setSbsNavigations();
    void setIonUtc();
    void setRtcm();
    void setRtcmDgps();
    void setRtcmSsr();
    void setReferenceStation();
    void showRtk();
    void showSat();
    void showEstimates();
    void showCovariance();
    void showObservations();
    void showNavigations();
    void showGlonassNavigations();
    void showSbsMessages();
    void showIonUtc();
    void showStream();
    void showSbsLong();
    void showSbsIono();
    void showSbsFast();
    void showSbsNavigations();
    void showRtcm();
    void showRtcmDgps();
    void showRtcmSsr();
    void showReferenceStation();

    void addConsole(const unsigned char *msg, int n, int mode);

public:
    explicit MonitorDialog(QWidget* parent);
    ~MonitorDialog();
};
//---------------------------------------------------------------------------
#endif
