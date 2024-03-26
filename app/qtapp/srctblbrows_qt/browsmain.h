//---------------------------------------------------------------------------
#ifndef browsmainH
#define browsmainH
//---------------------------------------------------------------------------
#include <QMainWindow>
#include <QFutureWatcher>

#include "ui_browsmain.h"

namespace Ui {
class MainForm;
}

class QShowEvent;
class QCloseEvent;
class StaListDialog;
class MapView;
class QTimer;
class MntpOptDialog;

//---------------------------------------------------------------------------
class MainForm : public QMainWindow
{
    Q_OBJECT

public:
    QStringList stationList;

    explicit MainForm(QWidget *parent = NULL);

protected:
    void showEvent(QShowEvent*);
    void closeEvent(QCloseEvent*);

public slots:
    void openSourceTable();
    void saveSourceTable();
    void viewStreamTable();
    void viewCasterTable();
    void viewNetTable();
    void viewSourceTable();
    void showAboutDialog();
    void showMap();
    void loadTimerExpired();
    void streamTableSelectItem(int ARow, int ACol);
    void streamTableShowMountpoint(int ARow, int ACol);
    void casterTableSelectItem(int ARow, int ACol);
    void showStationDialog();
    void updateCaster();
    void updateTable();
    void showMsg(const QString &);

private:
    void getCaster();
    void getTable();
    void updateMap();
    void updateEnable();
    void showTable();

    QString addressList, addressCaster, sourceTable, iniFile;
    float fontScale;
    MapView *mapView;
    StaListDialog *staListDialog;
    MntpOptDialog *mountPointDialog;
    QTimer *loadTimer;
    QFutureWatcher<char*> tableWatcher;
    QFutureWatcher<char*> casterWatcher;

    Ui::MainForm * ui;
};
//---------------------------------------------------------------------------
#endif
