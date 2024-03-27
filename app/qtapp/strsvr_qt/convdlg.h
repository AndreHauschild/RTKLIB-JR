//---------------------------------------------------------------------------
#ifndef convdlgH
#define convdlgH
//---------------------------------------------------------------------------
#include <QDialog>
#include "ui_convdlg.h"

class QShowEvent;

//---------------------------------------------------------------------------
class ConvDialog : public QDialog, private Ui::ConvDialog
{
    Q_OBJECT

public slots:
    void btnOkClicked();
    void updateEnable(void);

protected:
    void showEvent(QShowEvent*);

public:
    QString conversionMessage, conversionOptions, antennaType, receiverType;
    int conversionEnabled, conversionInputFormat, conversionOutputFormat, stationId;
    double antennaPosition[3], antennaOffset[3];

    explicit ConvDialog(QWidget *parent);
};
//---------------------------------------------------------------------------
#endif
