//---------------------------------------------------------------------------
#ifndef mapoptdlgH
#define mapoptdlgH
//---------------------------------------------------------------------------

#include <QDialog>

class QKeyEvent;
class QShowEvent;
class Plot;

namespace Ui {
class MapAreaDialog;
}

//---------------------------------------------------------------------------
class MapOptDialog : public QDialog
{
    Q_OBJECT

public:
    MapOptDialog(Plot * plot, QWidget* parent);
    void setMapSize(const QImage &mapImage);
    int getMapSize(int);
    bool getMapScaleEqual();
    double getMapScaleX();
    double getMapScaleY();
    double getMapLatitude();
    double getMapLongitude();
    int getPointType();

    void readMapTag(const QString &file);

protected:
    void showEvent(QShowEvent*);

protected slots:
    void saveTag();
    void btnCenterClicked();
    void scaleEqualChanged();
    void updateMap();
    void updateEnable();

private:
    Ui::MapAreaDialog *ui;
    Plot *plot;
};

//---------------------------------------------------------------------------
#endif
