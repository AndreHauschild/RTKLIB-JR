//---------------------------------------------------------------------------

#ifndef vmapdlgH
#define vmapdlgH
//---------------------------------------------------------------------------
#include <QDialog>
#include <QSettings>

#include "rtklib.h"

namespace Ui {
class VecMapDialog;
}

class Plot;
class QRadioButton;
class QCheckBox;

//---------------------------------------------------------------------------
class VecMapDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VecMapDialog(Plot * plot, QWidget *parent);

    QColor getMapColor(int);
    QColor getMapColorF(int);

    void loadOptions(QSettings &);
    void saveOptions(QSettings &);

public slots:
    void selectColor();
    void saveClose();
    void moveUp();
    void moveDown();

protected:
    void showEvent (QShowEvent *event);

private:
    QColor mapColor[24]; //0-11: line, 12-23: fill
    QColor color[24]; //0-11: line, 12-23: fill
    QList<QRadioButton*> rBLayer;
    QList<QCheckBox*> cBVisible;
    QList<QPushButton*> btnColor;
    QList<QPushButton*> btnColorf;
    gisd_t *gis_data[MAXGISLAYER];

    Plot *plot;
    Ui::VecMapDialog *ui;
};
//---------------------------------------------------------------------------
#endif
