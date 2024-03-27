//---------------------------------------------------------------------------
#ifndef gmviewH
#define gmviewH
//---------------------------------------------------------------------------
#include <QDialog>
#include <QTimer>

#include "ui_mapview.h"

#define NUM_MARK 256

class QWebEngineView;
class QWebChannel;
class QResizeEvent;
class QShowEvent;
class MapViewOptDialog;

//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
class MapView : public QDialog, private Ui::MapView
{
    Q_OBJECT

public slots:
    void btnZoomOutClicked();
    void btnZoomInClicked();
    void btnCenterClicked();
    void pageLoaded(bool);

    void showMapLL(void);
    void showMapGM(void);
    void setView(int map, double lat, double lon, int zoom);
    void btnOptionsClicked();
    void mapSelect1Clicked();
    void mapSelect2Clicked();
    void timerLLTimer();
    void timerGMTimer();

protected:
     void showEvent(QShowEvent*);

private:
    QWebEngineView *webBrowser;
    MapViewPageState *pageState;
    QWebChannel *webChannel;
    bool loaded;

    QString mapStrings[6][3];
    int selectedMap;

    double center_latitude, center_longitude;
    int markState[NUM_MARK];
    double markPosition[NUM_MARK][2];
    QString markTitle[NUM_MARK];
    QString markMessage[NUM_MARK];
    int highlightedMark;

    QTimer timerLL, timerGM;

    MapViewOptDialog *mapViewOptDialog;

    void updateMap(void);
    void selectMap(int map);
    int  setState(int map);
    void execFunction(int map, const QString &func);

public:
    QString apiKey;

    explicit MapView(QWidget *parent = NULL);
    void showMap(int map);
    void addMark(int map, int index, double lat, double lon, const QString& title, const QString& msg);
    void setCenter(double lat, double lon);
    void setMark(int index, const QString &title, double lat, double lon);
    void showMark(int index);
    void hideMark(int index);
    void highlightMark(int index);
    void clearMark();
    bool isLoaded();
};
//---------------------------------------------------------------------------
#endif
