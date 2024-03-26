//---------------------------------------------------------------------------
#ifndef tspandlgH
#define tspandlgH
//---------------------------------------------------------------------------
#include <QDialog>

#include "ui_tspandlg.h"

#include "rtklib.h"

class QShowEvent;

//---------------------------------------------------------------------------
class SpanDialog : public QDialog, public Ui::SpanDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent*);

    int timeValid[3];

public slots:
    void showStartTimeDialog();
    void showEndTimeDialog();

private:
    void updateEnable();

public:

    gtime_t getStartTime();
    void setStartTime(gtime_t);

    gtime_t getEndTime();
    void setEndTime(gtime_t);

    double getTimeInterval();
    void setTimeInterval(double);

    int getTimeEnable(int i);
    void setTimeEnable(int i, bool enable);

    int getTimeValid(int i);
    void setTimeValid(int i, int valid);

    explicit SpanDialog(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
