//---------------------------------------------------------------------------

#include <QShowEvent>
#include <QDateTime>
#include <QTimeZone>

#include "rtklib.h"
#include "plotmain.h"
#include "tspandlg.h"
#include "timedlg.h"

//---------------------------------------------------------------------------
SpanDialog::SpanDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    for (int i = 0; i < 3; i++) {
        timeValid[i] = true;
	}

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SpanDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SpanDialog::reject);
    connect(cBTimeEndEnabled, &QCheckBox::clicked, this, &SpanDialog::updateEnable);
    connect(cBTimeIntervalEnabled, &QCheckBox::clicked, this, &SpanDialog::updateEnable);
    connect(cBTimeStartEnabled, &QCheckBox::clicked, this, &SpanDialog::updateEnable);
    connect(btnTimeStart, &QPushButton::clicked, this, &SpanDialog::showStartTimeDialog);
    connect(btnTimeEnd, &QPushButton::clicked, this, &SpanDialog::showEndTimeDialog);
}
//---------------------------------------------------------------------------
void SpanDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    updateEnable();
}
//---------------------------------------------------------------------------
void SpanDialog::updateEnable()
{
    dTTimeStart->setEnabled(cBTimeStartEnabled->isChecked() && timeValid[0]);
    dTTimeEnd->setEnabled(cBTimeEndEnabled->isChecked() && timeValid[1]);
    sBTimeInterval->setEnabled(cBTimeIntervalEnabled->isChecked() && timeValid[2]);
    cBTimeStartEnabled->setEnabled(timeValid[0] == 1);
    cBTimeEndEnabled->setEnabled(timeValid[1] == 1);
    cBTimeIntervalEnabled->setEnabled(timeValid[2] == 1);
}
//---------------------------------------------------------------------------
void SpanDialog::showStartTimeDialog()
{
    TimeDialog * timeDialog = new TimeDialog(this);

    gtime_t time;
    time.time = dTTimeStart->dateTime().toUTC().toSecsSinceEpoch();
    time.sec = dTTimeStart->dateTime().toUTC().time().msec() / 1000;
    timeDialog->setTime(time);

    timeDialog->exec();
}
//---------------------------------------------------------------------------
void SpanDialog::showEndTimeDialog()
{
    TimeDialog * timeDialog = new TimeDialog(this);

    gtime_t time;
    time.time = dTTimeEnd->dateTime().toSecsSinceEpoch();
    time.sec = dTTimeEnd->dateTime().time().msec() / 1000;
    timeDialog->setTime(time);

    timeDialog->exec();
}
//---------------------------------------------------------------------------
gtime_t SpanDialog::getStartTime()
{
    QDateTime start(dTTimeStart->dateTime());
    gtime_t timeStart;

    timeStart.time = start.toSecsSinceEpoch();
    timeStart.sec = start.time().msec() / 1000;

    return timeStart;
}
//---------------------------------------------------------------------------
void SpanDialog::setStartTime(gtime_t timeStart)
{
    QDateTime start = QDateTime::fromSecsSinceEpoch(timeStart.time, QTimeZone::utc());

    start = start.addMSecs(timeStart.sec * 1000);

    dTTimeStart->setDateTime(start);
}
//---------------------------------------------------------------------------
gtime_t SpanDialog::getEndTime()
{
    QDateTime end(dTTimeEnd->dateTime());
    gtime_t timeEnd;

    timeEnd.time = end.toSecsSinceEpoch();
    timeEnd.sec = end.time().msec() / 1000;

    return timeEnd;
}
//---------------------------------------------------------------------------
void SpanDialog::setEndTime(gtime_t timeEnd)
{
    QDateTime end = QDateTime::fromSecsSinceEpoch(timeEnd.time, QTimeZone::utc());
    end = end.addMSecs(timeEnd.sec * 1000);

    dTTimeEnd->setDateTime(end);
}
//---------------------------------------------------------------------------
double SpanDialog::getTimeInterval()
{
    return sBTimeInterval->value();
}
//---------------------------------------------------------------------------
void SpanDialog::setTimeInterval(double timeInterval)
{
    sBTimeInterval->setValue(timeInterval);
}
//---------------------------------------------------------------------------
int SpanDialog::getTimeEnable(int i)
{
    switch(i)
    {
        case 0: return cBTimeStartEnabled->isChecked();
        case 1: return cBTimeEndEnabled->isChecked();
        case 2: return cBTimeIntervalEnabled->isChecked();
        default: return -1;
    }
}
//---------------------------------------------------------------------------
void SpanDialog::setTimeEnable(int i, bool enable)
{
    switch(i)
    {
        case 0: cBTimeStartEnabled->setChecked(enable);break;
        case 1: cBTimeEndEnabled->setChecked(enable);break;
        case 2: cBTimeIntervalEnabled->setChecked(enable);break;
    }
}
//---------------------------------------------------------------------------
int SpanDialog::getTimeValid(int i)
{
    return timeValid[i];
}
//---------------------------------------------------------------------------
void SpanDialog::setTimeValid(int i, int valid)
{
    timeValid[i] = valid;

    updateEnable();
}
//---------------------------------------------------------------------------

