//---------------------------------------------------------------------------
// gmview.c: map view
//---------------------------------------------------------------------------
#ifdef QWEBKIT
#include <QWebView>
#include <QWebFrame>
#include <QWebElement>
#endif
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
#include "plotmain.h"
#include "mapview.h"
#include "rtklib.h"


// instance of Plot --------------------------------------------------------
extern Plot *plot;

#define RTKLIB_GM_TEMP ":/html/rtkplot_gm.htm"
#define RTKLIB_GM_FILE "rtkplot_gm_a.htm"
#define RTKLIB_LL_TEMP ":/html/rtkplot_ll.htm"
#define RTKLIB_LL_FILE "rtkplot_ll_a.htm"
#define URL_GM_API     "http://maps.google.com/maps/api/js"
#define MAP_OPACITY    0.8
#define INIT_ZOOM      12  // initial zoom level

//---------------------------------------------------------------------------
MapView::MapView(QWidget *parent)
    : QDialog(parent)
{
    loaded = false;
    setupUi(this);
    
    selectedMap=0;
    center_latitude=center_longitude=0.0;
    for (int i=0;i<2;i++) {
        markState[0]=markState[1]=0;
        markPosition[i][0]=markPosition[i][1]=0.0;
    }
    mapViewOptDialog = new MapViewOptDialog(this);

    connect(btnClose, SIGNAL(clicked(bool)), this, SLOT(btnCloseClicked()));
    connect(btnOptions, SIGNAL(clicked(bool)), this, SLOT(btnOptionsClicked()));
    connect(btnShrink, SIGNAL(clicked(bool)), this, SLOT(btnZoomOutClicked()));
    connect(btnExpand, SIGNAL(clicked(bool)), this, SLOT(btnZoomInClicked()));
    connect(btnSync, SIGNAL(clicked(bool)), this, SLOT(btnSyncClicked()));
    connect(&timerLL, SIGNAL(timeout()), this, SLOT(timer1Timer()));
    connect(&timerGM, SIGNAL(timeout()), this, SLOT(timer2Timer()));
    connect(rBMapSelect1, SIGNAL(clicked(bool)), this, SLOT(mapSelect1Clicked()));
    connect(rBMapSelect2, SIGNAL(clicked(bool)), this, SLOT(mapSelect2Clicked()));

#ifdef QWEBKIT
    WebBrowser = new QWebView(Panel2);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(WebBrowser);
    Panel2->setLayout(layout);
#endif
#ifdef QWEBENGINE
    webBrowser = new QWebEngineView(panel2);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(webBrowser);
    panel2->setLayout(layout);
    pageState = new MapViewPageState(this);

    connect(webBrowser, SIGNAL(loadFinished(bool)), this, SLOT(pageLoaded(bool)));
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
void MapView::btnCloseClicked()
{
    close();
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
    mapViewOptDialog->move(x()+width()/2-mapViewOptDialog->width()/2,
                           y()+height()/2-mapViewOptDialog->height()/2);

    mapViewOptDialog->apiKey=plot->apiKey;
    for (int i=0;i<6;i++) for (int j=0;j<3;j++) {
        mapViewOptDialog->mapStrings[i][j]=plot->mapStreams[i][j];
    }
    if (mapViewOptDialog->exec()!=QDialog::Accepted) return;

    plot->apiKey=mapViewOptDialog->apiKey;
    for (int i=0;i<6;i++) for (int j=0;j<3;j++) {
        plot->mapStreams[i][j]=mapViewOptDialog->mapStrings[i][j];
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
void MapView::btnSyncClicked()
{
    if (btnSync->isChecked()) {
        setCenter(center_latitude,center_longitude);
    }
}
//---------------------------------------------------------------------------
void MapView::resizeEvent(QResizeEvent *)
{
    if (btnSync->isChecked()) setCenter(center_latitude, center_longitude);
}
//---------------------------------------------------------------------------
void MapView::showMap(int map)
{
    if (map==0) {
        showMapLL();
    }
    else if (map==1) {
        showMapGM();
    }
    else {
        updateMap();
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
        out<<buff;
        if (buff.contains("<script src=\"qrc:/leaflet/leaflet.js\"></script>")) out << "<script type=\"text/javascript\" src=\"https://getfirebug.com/firebug-lite.js\"></script>\n";
        if (!buff.contains("// start map tiles")) continue;
        for (i=0,j=1;i<6;i++) {
            if (plot->mapStreams[i][0]=="") continue;
            QString title=plot->mapStreams[i][0];
            QString url  =plot->mapStreams[i][1];
            QString attr =plot->mapStreams[i][2];

            out << QString("var tile%1 = L.tileLayer('%2', {\n").arg(j).arg(url);
            out << QString("  attribution: \"<a href='%1' target='_blank'>%2</a>\",\n")
                    .arg(attr).arg(title);
            out << QString("  opacity: %1});\n").arg(MAP_OPACITY,0,'f',1);
            j++;
        }
        out << "var basemaps = {";
        for (i=0,j=1;i<6;i++) {
            if (plot->mapStreams[i][0]=="") continue;
            QString title=plot->mapStreams[i][0];
            out << QString("%1\"%2\":tile%3").arg((j==1)?"":",").arg(title).arg(j);
            j++;
        }
        out << "};\n";
    }
    ifp.close();


#ifdef QWEBKIT
    WebBrowser->load(QUrl::fromLocalFile(ofile));
    WebBrowser->show();
    loaded = true;
#endif
#ifdef QWEBENGINE
    webBrowser->setHtml(pageSource);
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("state"), pageState);

    webBrowser->page()->setWebChannel(channel);

    webBrowser->show();
#endif
    
    timerLL.start();
}
//---------------------------------------------------------------------------
void MapView::timer1Timer()
{
    if (!setState(0)) return;
    
    setView(0,center_latitude,center_longitude,INIT_ZOOM);
    addMark(0,1,markPosition[0][0],markPosition[0][1],markState[0]);
    addMark(0,2,markPosition[1][0],markPosition[1][1],markState[1]);
    
    timerLL.stop();
}
void MapView::showMapGM(void)
{
    QString pageSource;
    int p;

    QFile ifp(RTKLIB_GM_TEMP);

    if (!ifp.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&pageSource);
    while (!ifp.atEnd()) {
        QString buff = ifp.readLine();

        if ((!plot->apiKey.isEmpty())&&(p=buff.indexOf(QLatin1String(URL_GM_API)))!=-1) {
            p+=strlen(URL_GM_API);
            buff.insert(p, QString("?key=%1").arg(plot->apiKey));
        }

        out<<buff;
    }
    ifp.close();

#ifdef QWEBKIT
    WebBrowser->load(QUrl::fromLocalFile(ofile));
    WebBrowser->show();
    loaded = true;
#endif
#ifdef QWEBENGINE
    webBrowser->setHtml(pageSource);
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("state"), pageState);

    webBrowser->page()->setWebChannel(channel);

    webBrowser->show();
#endif
    
    timerGM.start();

}
//---------------------------------------------------------------------------
void MapView::timer2Timer()
{
    if (!setState(1)) return;
    
    setView(1,center_latitude,center_longitude,INIT_ZOOM);
        addMark(1,1,markPosition[0][0],markPosition[0][1],markState[0]);
        addMark(1,2,markPosition[1][0],markPosition[1][1],markState[1]);
        timerGM.stop();
}

//---------------------------------------------------------------------------
void MapView::setView(int map, double lat, double lon, int zoom)
{
    execFunction(map, QString("SetView(%1,%2,%3)").arg(lat, 0, 'f', 9).arg(lon, 0, 'f', 9).arg(zoom));
}

//---------------------------------------------------------------------------
void MapView::addMark(int map, int index, double lat, double lon,
                int state)
{
    QString func = QString("AddMark(%1,%2,'SOL%3','SOLUTION %4')").arg(lat, 0, 'f', 9).arg(lon, 0, 'f', 9).arg(index).arg(index);

   execFunction(map,func);
   if (state) func = QString("ShowMark('SOL%1')").arg(index);
   else       func = QString("HideMark('SOL%1')").arg(index);
   execFunction(map,func);
}
//---------------------------------------------------------------------------
void MapView::updateMap(void)
{
    setCenter(center_latitude,center_longitude);
    for (int i=0;i<2;i++) {
        setMark(i+1,markPosition[i][0],markPosition[i][1]);
        if (markState[i]) showMark(i+1); else hideMark(i+1);
    }
}
//---------------------------------------------------------------------------
void MapView::selectMap(int map)
{
    selectedMap=map;
    showMap(map);
}

//---------------------------------------------------------------------------
void MapView::setCenter(double lat, double lon)
{
    QString func=QString("SetCent(%1,%2)").arg(lat, 0, 'f', 9).arg(lon, 0, 'f', 9);
    center_latitude = lat; center_longitude = lon;

    if (btnSync->isChecked()) {
        execFunction(selectedMap,func);
    }
}
//---------------------------------------------------------------------------
void MapView::setMark(int index, double lat, double lon)
{
    QString func = QString("PosMark(%1,%2,'SOL%3')").arg(lat, 0, 'f', 9).arg(lon, 0, 'f', 9).arg(index);

    markPosition[index-1][0]=lat;
    markPosition[index-1][1]=lon;
    
    execFunction(selectedMap,func);

}
//---------------------------------------------------------------------------
void MapView::showMark(int index)
{
    QString func = QString("ShowMark('SOL%1')").arg(index);

    markState[index-1]=1;
    execFunction(selectedMap,func);
}
//---------------------------------------------------------------------------
void MapView::hideMark(int index)
{
    QString func = QString("HideMark('SOL%1')").arg(index);

    markState[index-1]=0;
    execFunction(selectedMap,func);
}
//---------------------------------------------------------------------------
int MapView::setState(int map)
{
    Q_UNUSED(map)
#ifdef QWEBKIT
    QWebElement ele;
    int state = 0;

    if (!WebBrowser->page()) return 0;
    if (!WebBrowser->page()->mainFrame()) return 0;

    QWebFrame *frame = WebBrowser->page()->mainFrame();

    qDebug() << frame;

    ele = frame->findFirstElement("#state");

    if (ele.isNull()) return 0;
    if (!ele.hasAttribute("value")) return 0;

    state = ele.attribute("value").toInt();

	return state;
#else
#ifdef QWEBENGINE
    if (!loaded) return 0;
    return pageState->getText().toInt();
#else
    return 0;
#endif
#endif
}
//---------------------------------------------------------------------------
void MapView::execFunction(int map, const QString &func)
{
    Q_UNUSED(map)
#ifdef QWEBKIT
    if (!WebBrowser->page()) return;
    if (!WebBrowser->page()->mainFrame()) return;

    QWebFrame *frame = WebBrowser->page()->mainFrame();

    frame->evaluateJavaScript(func);
#else
#ifdef QWEBENGINE
    if (!loaded) return;

    QWebEnginePage *page = webBrowser->page();
    if (page == NULL) return;

    page->runJavaScript(func);
#else
    Q_UNUSED(func)
#endif
#endif
}
//---------------------------------------------------------------------------
