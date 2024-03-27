//---------------------------------------------------------------------------

#ifndef plotoptH
#define plotoptH
//---------------------------------------------------------------------------
#include <QDialog>
#include "ui_plotopt.h"

class Plot;
class RefDialog;

//---------------------------------------------------------------------------
class PlotOptDialog : public QDialog, private Ui::PlotOptDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent*);

public slots:
    void btnOKClicked();
    void btnColor1Clicked();
    void btnColor2Clicked();
    void btnColor3Clicked();
    void btnColor4Clicked();
    void btnReferencePositionClicked();
    void originChanged();
    void autoScaleChanged();
    void mColorClicked();
    void btnFontClicked();
    void receiverPositionChanged();
    void btnTLEFileClicked();
    void btnShapeFileClicked();
    void btnTLESatelliteFileClicked();
    void btnTLEViewClicked();
    void btnTLESatelliteViewClicked();
    void timeSyncClicked();

private:
    void updateFont(void);
    void updateEnable(void);
    QColor mColor[2][8]; // {{mark1 0-7},{mark2 0-7}}
    QColor cColor[4];    // {background,grid,text,line}
    QFont fontOption;

public:
    Plot *plot;
    RefDialog *refDialog;

    explicit PlotOptDialog(QWidget *parent=NULL);
};
//---------------------------------------------------------------------------
#endif
