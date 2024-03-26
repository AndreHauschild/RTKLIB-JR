//---------------------------------------------------------------------------
#ifndef svroptdlgH
#define svroptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class SvrOptDialog;
}

class QShowEvent;
class RefDialog;
//---------------------------------------------------------------------------
class SvrOptDialog : public QDialog
{
    Q_OBJECT

public slots:
    void btnOkClicked();
    void positionSelect();
    void localDirectorySelect();
    void logFileSelect();

protected:
    void showEvent(QShowEvent*);
    RefDialog *refDialog;

private:
    void updateEnable();
    Ui::SvrOptDialog *ui;

public:
    QString stationPositionFile, exeDirectory, localDirectory, proxyAddress;
    QString antennaType, receiverType, logFile;
    int serverOptions[6], traceLevel, nmeaRequest, fileSwapMargin, stationId, stationSelect, relayBack;
    int progressBarRange;
    double antennaPosition[3], antennaOffsets[3];

    explicit SvrOptDialog(QWidget *parent);
};
//---------------------------------------------------------------------------
#endif
