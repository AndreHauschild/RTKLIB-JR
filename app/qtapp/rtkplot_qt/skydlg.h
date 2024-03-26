//---------------------------------------------------------------------------
#ifndef skydlgH
#define skydlgH
//---------------------------------------------------------------------------
#include <QDialog>
#include <QSettings>

namespace Ui {
class SkyImgDialog;
}
class QShowEvent;
class Plot;

//---------------------------------------------------------------------------
class SkyImgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SkyImgDialog(Plot *plot, QWidget *parent = NULL);
    void setImage(QImage &, double w, double h);
    void readSkyTag(const QString &file);

    int* getSkySize();
    bool getSkyDistortionCorrection();
    bool getSkyElevationMask();
    int getSkyResample();
    bool getSkyFlip();
    bool getSkyBinarize();
    double* getSkyCenter();
    double getSkyScale();
    double getSkyScaleR();
    double* getSkyFOV();
    double getSkyDistortion(int);
    double getSkyBinThres1();
    double getSkyBinThres2();

protected slots:
    void saveSkyImageTag();
    void loadSkyImageTag();
    void generateMask();
    void updateSkyEnabled();
    void updateEnable();

private:
    void updateSky();

    Plot *plot;
    Ui::SkyImgDialog *ui;

    // sky image options
    int skySize[2];
    double skyScaleR;

	
};
//---------------------------------------------------------------------------
#endif
