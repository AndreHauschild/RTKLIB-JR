//---------------------------------------------------------------------------
// gmview.c: map view
//---------------------------------------------------------------------------
#ifdef QWEBENGINE
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebChannel>
#include <QFile>
#endif
#include <QShowEvent>
#include <QFile>
#include <QTextStream>
#include "mapviewopt.h"
#include "mapview.h"
#include <cmath>

#define RTKLIB_GM_TEMP ":/html/rtklib_gm.htm"
#define RTKLIB_GM_FILE "rtklib_gm_a.htm"
#define RTKLIB_LL_TEMP ":/html/rtklib_ll.htm"
#define RTKLIB_LL_FILE "rtklib_ll_a.htm"
#define URL_GM_API     "http://maps.google.com/maps/api/js"
#define MAP_OPACITY    0.8
#define INIT_ZOOM      8  // initial zoom level

//---------------------------------------------------------------------------
MapView::MapView(QWidget *parent)
    : QDialog(parent)
{
    loaded = false;
    setupUi(this);

    selectedMap = 0;
    center_latitude = center_longitude = 0.0;
    highlightedMark = -1;
    for (int i = 0; i < 256; i++) {
        markState[0] = markState[1] = 0;
        markPosition[i][0] = markPosition[i][1] = NAN;
    }
    mapViewOptDialog = new MapViewOptDialog(this);

    connect(btnClose, &QPushButton::clicked, this, &MapView::close);
    connect(btnOptions, &QPushButton::clicked, this, &MapView::btnOptionsClicked);
    connect(btnShrink, &QPushButton::clicked, this, &MapView::btnZoomOutClicked);
    connect(btnExpand, &QPushButton::clicked, this, &MapView::btnZoomInClicked);
    connect(btnCenter, &QPushButton::clicked, this, &MapView::btnCenterClicked);
    connect(&timerLL, &QTimer::timeout, this, &MapView::timerLLTimer);
    connect(&timerGM, &QTimer::timeout, this, &MapView::timerGMTimer);
    connect(rBMapSelect1, &QRadioButton::clicked, this, &MapView::mapSelect1Clicked);
    connect(rBMapSelect2, &QRadioButton::clicked, this, &MapView::mapSelect2Clicked);

    mapStrings[0][0] = "OpenStreetMap";
    mapStrings[0][1] = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
    mapStrings[0][2] = "https://osm.org/copyright";

    QHBoxLayout *layout = new QHBoxLayout();
    panel2->setContentsMargins(0, 0, 0, 0);
    panel2->setLayout(layout);
#ifdef QWEBENGINE
    webBrowser = new QWebEngineView(panel2);
    layout->addWidget(webBrowser);
    pageState = new MapViewPageState(this);
    webChannel = new QWebChannel(this);
    webChannel->registerObject(QStringLiteral("state"), pageState);

    connect(webBrowser, &QWebEngineView::loadFinished, this, &MapView::pageLoaded);
#else
    QLabel *label = new QLabel();
    label->setText("QWebEngine is not available to show a map.");
    layout->addWidget(label);
#endif
}
//---------------------------------------------------------------------------
void MapView::showEvent(QShowEvent*)
{
    rBMapSelect1->setChecked(!selectedMap);
    rBMapSelect2->setChecked(selectedMap);
    selectMap(selectedMap);
    showMap(selectedMap);
}

//---------------------------------------------------------------------------
void MapView::mapSelect1Clicked()
{
    selectMap(0);
}
//---------------------------------------------------------------------------
void MapView::mapSelect2Clicked()
{
    selectMap(1);
}
//---------------------------------------------------------------------------
void MapView::btnOptionsClicked()
{
    mapViewOptDialog->move(x() + width() / 2 - mapViewOptDialog->width() / 2,
                           y() + height() / 2 - mapViewOptDialog->height() / 2);

    mapViewOptDialog->apiKey = apiKey;
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 3; j++) {
            mapViewOptDialog->mapStrings[i][j] = mapStrings[i][j];
        }
    if (mapViewOptDialog->exec()!=QDialog::Accepted) return;

    apiKey = mapViewOptDialog->apiKey;
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 3; j++) {
            mapStrings[i][j] = mapViewOptDialog->mapStrings[i][j];
        }

    showMap(selectedMap);
}
//---------------------------------------------------------------------------
void MapView::pageLoaded(bool ok)
{
    if (!ok) return;

#ifdef QWEBENGINE
    QFile webchannel(":/html/qwebchannel.js");
    webchannel.open(QIODevice::ReadOnly);
    webBrowser->page()->runJavaScript(webchannel.readAll());
    webBrowser->page()->runJavaScript("new QWebChannel(qt.webChannelTransport,function(channel) {channel.objects.state.text=document.getElementById('state').value;});");
#endif

    loaded = true;
}
//---------------------------------------------------------------------------
void MapView::btnZoomOutClicked()
{
    execFunction(selectedMap,"ZoomOut()");
}
//---------------------------------------------------------------------------
void MapView::btnZoomInClicked()
{
    execFunction(selectedMap,"ZoomIn()");
}
//---------------------------------------------------------------------------
void MapView::btnCenterClicked()
{
        setCenter(center_latitude,center_longitude);
}
//---------------------------------------------------------------------------
void MapView::showMap(int map)
{
    if (map == 0) {
        showMapLL();
    }
    else if (map == 1) {
        showMapGM();
    }
}
//---------------------------------------------------------------------------
void MapView::showMapLL(void)
{
    QString pageSource;
    int i, j;

    QFile ifp(RTKLIB_LL_TEMP);

    if (!ifp.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&pageSource);
    while (!ifp.atEnd()) {
        QString buff = ifp.readLine();
        out << buff;
        if (buff.contains("<script src=\"qrc:/leaflet/leaflet.js\"></script>")) out << "<script type=\"text/javascript\" src=\"https://getfirebug.com/firebug-lite.js\"></script>\n";
        if (!buff.contains("// start map tiles")) continue;
        for (i = 0, j = 1; i < 6; i++) {
            if (mapStrings[i][0] == "") continue;
            QString title = mapStrings[i][0];
            QString url = mapStrings[i][1];
            QString attr = mapStrings[i][2];

            out << QString("var tile%1 = L.tileLayer('%2', {\n").arg(j).arg(url);
            out << QString("  attribution: \"<a href='%1' target='_blank'>%2</a>\",\n").arg(attr).arg(title);
            out << QString("  opacity: %1});\n").arg(MAP_OPACITY,0,'f',1);
            j++;
        }
        out << "var basemaps = {";
        for (i = 0, j = 1; i < 6; i++) {
            if (mapStrings[i][0] == "") continue;
            QString title = mapStrings[i][0];
            out << QString("%1\"%2\":tile%3").arg((j == 1) ? "" : ",").arg(title).arg(j);
            j++;
        }
        out << "};\n";
    }
    ifp.close();

#ifdef QWEBENGINE
    webBrowser->setHtml(pageSource);
    webBrowser->page()->setWebChannel(webChannel);
    webBrowser->show();
    loaded = false;
#endif

    timerLL.start();
}
//---------------------------------------------------------------------------
void MapView::timerLLTimer()
{
    if (!setState(0)) return;

    setView(0, center_latitude, center_longitude, INIT_ZOOM);
    updateMap();

    timerLL.stop();
}
void MapView::showMapGM(void)
{
    QString pageSource;

    QFile ifp(RTKLIB_GM_TEMP);

    if (!ifp.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&pageSource);
    while (!ifp.atEnd()) {
        QString buff = ifp.readLine();

        if (!apiKey.isEmpty()) {
            buff = buff.replace(QStringLiteral("_insertKeyHere_"), apiKey);
        }

        out << buff;
    }
    ifp.close();

#ifdef QWEBENGINE
    webBrowser->setHtml(pageSource);
    webBrowser->page()->setWebChannel(webChannel);
    webBrowser->show();
    loaded = false;
#endif
    timerGM.start();

}
//---------------------------------------------------------------------------
void MapView::timerGMTimer()
{
    if (!setState(1)) return;

    setView(1, center_latitude, center_longitude, INIT_ZOOM);
    updateMap();

    timerGM.stop();
}

//---------------------------------------------------------------------------
void MapView::setView(int map, double lat, double lon, int zoom)
{
    execFunction(map, QString("SetView(%1,%2,%3)").arg(lat, 0, 'f', 9).arg(lon, 0, 'f', 9).arg(zoom));
}

//---------------------------------------------------------------------------
void MapView::updateMap(void)
{
    setCenter(center_latitude, center_longitude);
    for (int i = 0; i < 255; i++) {
        if (std::isnan(markPosition[i][0]) || std::isnan(markPosition[i][0])) continue;
        addMark(selectedMap, i, markPosition[i][0], markPosition[i][1], markTitle[i], markMessage[i]);
        if (markState[i]) showMark(i);
        else hideMark(i);
    }
}
//---------------------------------------------------------------------------
void MapView::selectMap(int map)
{
    selectedMap = map;
    setView(map, center_latitude, center_longitude, INIT_ZOOM);
    showMap(map);
}

//---------------------------------------------------------------------------
void MapView::setCenter(double lat, double lon)
{
    QString func=QString("SetCent(%1,%2)").arg(lat, 0, 'f', 9).arg(lon, 0, 'f', 9);
    center_latitude = lat;
    center_longitude = lon;

    execFunction(selectedMap, func);
}
//---------------------------------------------------------------------------
void MapView::addMark(int map, int index, double lat, double lon, const QString &title, const QString &msg)
{
    QString func = QString("AddMark(%1,%2,'%3','%4')").arg(lat, 0, 'f', 9).arg(lon, 0, 'f', 9).arg(title).arg(msg);

    markTitle[index] = title;
    markMessage[index] = msg;
    markPosition[index][0] = lat;
    markPosition[index][1] = lon;
    markState[index] = 1;

    execFunction(map, func);
    func = QString("ShowMark('%1')").arg(title);
    execFunction(map, func);
}
//---------------------------------------------------------------------------
void MapView::setMark(int index, const QString &title, double lat, double lon)
{
    markTitle[index] = title;
    QString func = QString("PosMark(%1,%2,'%3')").arg(lat, 0, 'f', 9).arg(lon, 0, 'f', 9).arg(title);

    markPosition[index][0] = lat;
    markPosition[index][1] = lon;

    execFunction(selectedMap, func);

}
//---------------------------------------------------------------------------
void MapView::showMark(int index)
{
    QString title = markTitle[index];
    QString func = QString("ShowMark('%1')").arg(title);

    markState[index] = 1;
    execFunction(selectedMap,func);
}
//---------------------------------------------------------------------------
void MapView::hideMark(int index)
{
    QString title = markTitle[index];
    QString func = QString("HideMark('%1')").arg(title);

    markState[index] = 0;
    execFunction(selectedMap,func);
}
//---------------------------------------------------------------------------
void MapView::highlightMark(int index)
{
    QString title = markTitle[index];
    QString func = QString("HighlightMark('%1')").arg(title);

    highlightedMark = index;
    execFunction(selectedMap, func);
}
//---------------------------------------------------------------------------
void MapView::clearMark()
{
    QString func = QString("ClearMark()");

    highlightedMark = -1;
    for (int i = 0; i < 256; i++) {
        markState[0] = markState[1] = 0;
        markPosition[i][0] = markPosition[i][1] = NAN;
    }
    execFunction(selectedMap,func);
}
//---------------------------------------------------------------------------
int MapView::setState(int map)
{
    Q_UNUSED(map)
    if (!loaded) return 0;
    return pageState->getText().toInt();
}
//---------------------------------------------------------------------------
void MapView::execFunction(int map, const QString &func)
{
    Q_UNUSED(map)
    if (!loaded) return;

#ifdef QWEBENGINE
    QWebEnginePage *page = webBrowser->page();
    if (page == NULL) return;

    page->runJavaScript(func);
#endif
}
//---------------------------------------------------------------------------
bool MapView::isLoaded()
{
    return setState(1);
}
//---------------------------------------------------------------------------
