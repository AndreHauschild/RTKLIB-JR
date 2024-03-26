//---------------------------------------------------------------------------
#ifndef postoptH
#define postoptH
//---------------------------------------------------------------------------
#include <QDialog>

#include "ui_postopt.h"

#include "rtklib.h"
//---------------------------------------------------------------------------
class OptDialog : public QDialog, public Ui::OptDialog
{
    Q_OBJECT

public slots:

    void saveClose();
    void selectAntennaPcvFile();
    void selectIonosphereFile();
    void viewAntennaPcvFile();

    void loadOptions();
    void saveOptions();
    void showReferencePositionDialog();
    void showRoverPositionDialog();
    void viewStationPositionFile();
    void selectStationPositionFile();

    void referencePositionTypeChanged();
    void roverPositionTypeChanged();
    void getPosition(int type, QLineEdit **edit, double *pos);
    void setPosition(int type, QLineEdit **edit, double *pos);

    void selectSatellitePcvFile();
    void viewSatelitePcvFile();
    void selectGeoidDataFile();

    void viewDCBFile();
    void selectDCBFile();
    void showKeyDialog();
    void viewBLQFile();
    void selectBLQFile();
    void selectEOPFile();
    void viewEOPView();
    void showFrequenciesDialog();
    void showMaskDialog();
    void updateEnable();

protected:

    void showEvent(QShowEvent*);

private:

    snrmask_t snrMask;
    int roverPositionType, referencePositionType;

    void getOptions(void);
    void setOptions(void);
    void load(const QString &file);
    void save(const QString &file);
    void readAntennaList(void);
    void updateEnableExtErr(void);

public:
	
    explicit OptDialog(QWidget *parent);
};
//---------------------------------------------------------------------------
#endif
