//---------------------------------------------------------------------------
#ifndef gmviewH
#define gmviewH
//---------------------------------------------------------------------------
#include <QDialog>
#include <QTimer>

#include "ui_mapview.h"

#ifdef QWEBENGINE
class QWebEngineView;
class MapViewPageState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text MEMBER text NOTIFY textChanged)
public:
    explicit MapViewPageState(QObject *parent = NULL): QObject(parent){}
    QString getText() {return text;}
signals:
    void textChanged(const QString &text);
private:
    QString text;
};
#endif

class QResizeEvent;
class QShowEvent;
class MapViewOptDialog;
//---------------------------------------------------------------------------
class MapView : public QDialog, private Ui::MapView
{
    Q_OBJECT

public slots:
    void btnCloseClicked();
    void timer1Timer();
    void btnZoomOutClicked();
    void btnZoomInClicked();
    void btnSyncClicked();
    void pageLoaded(bool);

    void btnOptionsClicked();
    void mapSelect1Clicked();
    void mapSelect2Clicked();
    void timer2Timer();

protected:
    void resizeEvent(QResizeEvent*);
     void showEvent(QShowEvent*);

private:
#ifdef QWEBENGINE
    QWebEngineView *webBrowser;
    MapViewPageState *pageState;
#endif
    int markState[2];
    double center_latitude, center_longitude;
    double markPosition[2][2];
    QTimer timerLL, timerGM;
    bool loaded;

    MapViewOptDialog *mapViewOptDialog;

    void showMapLL(void);
    void showMapGM(void);
    void showMap(int map);
    void setView(int map, double lat, double lon, int zoom);
    void addMark(int map, int index, double lat, double lon, int state);
    void updateMap(void);
    void selectMap(int map);
    int  setState(int map);
    void execFunction(int map, const QString &func);

public:
    int selectedMap;

    explicit MapView(QWidget *parent = NULL);
    void setCenter(double lat, double lon);
    void setMark(int index, double lat, double lon);
    void showMark(int index);
    void hideMark(int index);
};
//---------------------------------------------------------------------------
#endif
