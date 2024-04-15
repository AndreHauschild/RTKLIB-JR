//---------------------------------------------------------------------------
#ifndef getmainH
#define getmainH
//---------------------------------------------------------------------------
#include <QDialog>
#include <QSettings>
#include <QTimer>
#include <QPixmap>
#include <QSystemTrayIcon>

#include "rtklib.h"

namespace Ui {
class MainForm;
}

class TextViewer;
class DownOptDialog;
class DownloadThread;
class TimeDialog;
class QComboBox;

//---------------------------------------------------------------------------
class MainForm : public QWidget
{
     Q_OBJECT

public:
    explicit MainForm(QWidget* parent);

    void showMessage(int i, const QString&);

protected:
    void  closeEvent(QCloseEvent *);
    void  showEvent(QShowEvent*);
    void  dragEnterEvent(QDragEnterEvent *event);
    void  dropEvent(QDropEvent * event);

    QString iniFilename;
    QTimer busyTimer;
    int timerCnt;

protected slots:
    void  showOptionsDialog();
    void  viewLogFile();
    void  download();
    void  openOutputDirectory();
    void  dataListSelectionChanged();
    void  selectOutputDirectory();
    void  localDirectoryCheckBoxClicked();
    void  showStationDialog();
    void  showKeyDialog();
    void  showAboutDialog();
    void  busyTimerTriggered();
    void  minimizeToTray();
    void  restoreFromTaskTray(QSystemTrayIcon::ActivationReason);
    void  testDownload();
    void  selectDeselectAllStations();
    void  downloadFinished();
    void  showStartTimeDetails();
    void  showStopTimeDetails();
    void  updateEnable();
    void  updateDataListWidget();
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
    DownOptDialog *downOptDialog;

    Ui::MainForm *ui;

    void  loadOptions();
    void  saveOptions();
    void  updateStationListLabel();
    void  panelEnable(int ena);
    void  getTime(gtime_t *ts, gtime_t *te, double *ti);
    int   selectUrl(url_t *urls);
    int   selectStation(char **stas);
    void  loadUrlList(const QString &file);
    void  loadStationFile(const QString &file);
    int   execCommand(const QString &cmd, const QStringList &opt);
    void  readHistory(QSettings &, const QString &key, QComboBox *);
    void  writeHistory(QSettings &, const QString &key, QComboBox *);
    void  addHistory(QComboBox *combo);
	
};
//---------------------------------------------------------------------------
#endif
