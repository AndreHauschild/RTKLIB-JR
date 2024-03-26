//---------------------------------------------------------------------------
#ifndef timedlgH
#define timedlgH
//---------------------------------------------------------------------------
#include <QDialog>

#include "rtklib.h"

namespace Ui {
class TimeDialog;
}

class QShowEvent;

//---------------------------------------------------------------------------
class TimeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TimeDialog(QWidget *parent = nullptr);
    void setTime(const gtime_t &time);

private:
    Ui::TimeDialog *ui;
};
//---------------------------------------------------------------------------
#endif
