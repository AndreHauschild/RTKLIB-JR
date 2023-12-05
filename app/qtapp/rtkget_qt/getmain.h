//---------------------------------------------------------------------------
#ifndef getmainH
#define getmainH
//---------------------------------------------------------------------------
#include <QDialog>
#include <QSettings>
#include <QTimer>
#include <QPixmap>
#include <QSystemTrayIcon>

#define INHIBIT_RTK_LOCK_MACROS
#include "rtklib.h"
#include "ui_getmain.h"

class TextViewer;
class DownloadThread;
class TimeDialog;

//---------------------------------------------------------------------------
class MainForm : public QWidget, public Ui::MainForm
{
     Q_OBJECT

protected:
    void  closeEvent(QCloseEvent *);

    void  showEvent(QShowEvent*);

    void  dragEnterEvent(QDragEnterEvent *event);
    void  dropEvent(QDropEvent * event);

public slots:
    void  btnOptionsClicked();
    void  btnLogClicked();
    void  btnDownloadClicked();
    void  btnFileClicked();
    void  dataListClicked();
    void  btnDirClicked();
    void  localDirClicked();
    void  btnStationsClicked();
    void  btnKeywordClicked();
    void  btnAboutClicked();
    void  busyTimerTriggered();
    void  btnTrayClicked();
    void  trayIconActivated(QSystemTrayIcon::ActivationReason);
    void  btnTestClicked();
    void  btnAllClicked();
    void  downloadFinished();
    void  btnTimeStartClicked();
    void  btnTimeStopClicked();
    void  updateEnable();
    void  updateType();
    void  updateMessage();

private:
    QStringList types;
    QStringList urls;
    QStringList locals;
    QPixmap images[8];
    QSystemTrayIcon trayIcon;
    DownloadThread *processingThread;
    TextViewer *viewer;
    TimeDialog *timeDialog;

    void  loadOptions();
    void  saveOptions();
    void  updateStationList();
    void  panelEnable(int ena);
    void  getTime(gtime_t *ts, gtime_t *te, double *ti);
    int   selectUrl(url_t *urls);
    int   selectStation(char **stas);
    void  loadUrl(QString file);
    void  loadStation(QString file);
    int   execCommand(const QString &cmd, const QStringList &opt);
    void  readHistory(QSettings &, QString key, QComboBox *);
    void  writeHistory(QSettings &, QString key, QComboBox *);
    void  addHistory(QComboBox *combo);
	
public:
    QString iniFilename;
    QString urlFile;
    QString logFile;
    QString stations;
    QString proxyAddr;
    int holdErr;
    int holdList;
    int columnCnt;
    int dateFormat;
    int traceLevel;
    int logAppend;
    int timerCnt;
    QTimer busyTimer;

    explicit MainForm(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
