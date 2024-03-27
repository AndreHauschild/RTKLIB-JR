//---------------------------------------------------------------------------
#ifndef svroptdlgH
#define svroptdlgH
//---------------------------------------------------------------------------
#include <QDialog>
#include "ui_svroptdlg.h"

class QShowEvent;
//---------------------------------------------------------------------------
class SvrOptDialog : public QDialog, public Ui::SvrOptDialog
{
    Q_OBJECT

public slots:
    void btnOkClicked();
    void btnPosClicked();
    void btnLocalDirClicked();
    void btnLogFileClicked();

protected:
    void showEvent(QShowEvent*);

private:
    void updateEnable(void);

public:
    QString stationPositionFile, exeDirectory, localDirectory, proxyAddress;
    QString antennaType, receiverType, logFile;
    int serverOptions[6], traceLevel, nmeaRequest, fileSwapMargin, stationId, stationSelect, relayBack;
    int progressBarRange;
    double antennaPos[3], antennaOffset[3];

    explicit SvrOptDialog(QWidget *parent);
};
//---------------------------------------------------------------------------
#endif
