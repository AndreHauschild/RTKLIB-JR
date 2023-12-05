//---------------------------------------------------------------------------
// rtklaunch_qt : rtklib app launcher
//
//          Copyright (C) 2013-2016 by T.TAKASU, All rights reserved.
//          ported to Qt by Jens Reimann
//
// options : rtklib launcher [-t title][-tray]
//
//           -t title   window title
//           -tray      start as task tray icon
//
// version : $Revision:$ $Date:$
// history : 2013/01/10  1.1 new
//---------------------------------------------------------------------------

#include <QSettings>
#include <QFileInfo>
#include <QShowEvent>
#include <QCommandLineParser>
#include <QProcess>
#include <QMenu>

#include "rtklib.h"
#include "launchmain.h"
#include "launchoptdlg.h"

//---------------------------------------------------------------------------

#define BTN_SIZE        42
#define BTN_COUNT       8
#define MAX(x, y)        ((x) > (y) ? (x) : (y))

MainForm *mainForm;

//---------------------------------------------------------------------------
MainForm::MainForm(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    mainForm = this;

    QString prgFilename = QApplication::applicationFilePath();
    QFileInfo prgFileInfo(prgFilename);
    iniFile = prgFileInfo.absolutePath() + "/" + prgFileInfo.baseName() + ".ini";
    option = 0;
    minimize = 0;

    launchOptDlg = new LaunchOptDialog(this);

    setWindowTitle(tr("RTKLIB v.%1 %2").arg(VER_RTKLIB).arg(PATCH_LEVEL));
    setWindowIcon(QIcon(":/icons/rtk9"));

    QCoreApplication::setApplicationName("rtklaunch_qt");
    QCoreApplication::setApplicationVersion("1.0");

    QSettings settings(iniFile, QSettings::IniFormat);
    option =  settings.value("pos/option", 0).toInt();
    minimize =  settings.value("pos/minimize", 1).toInt();
    btnRtklib->setStatusTip("RTKLIB v." VER_RTKLIB " " PATCH_LEVEL);

    QCommandLineParser parser;
    parser.setApplicationDescription("rtklib application launcher Qt");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption titleOption("t",
                       QCoreApplication::translate("main", "Set Window Title"),
                       QCoreApplication::translate("main", "title"));
    parser.addOption(titleOption);

    QCommandLineOption trayOption(QStringList() << "tray",
                      QCoreApplication::translate("main", "only show tray icon"));
    parser.addOption(trayOption);

    QCommandLineOption miniOption(QStringList() << "min",
                      QCoreApplication::translate("main", "start minimized"));
    parser.addOption(miniOption);


    parser.process(*QApplication::instance());

    bool tray = parser.isSet(trayOption);

    if (parser.isSet(titleOption))
        setWindowTitle(parser.value(titleOption));

    if (tray) {
        setVisible(false);
        trayIcon.setVisible(true);
    }

    if (parser.isSet(miniOption)) minimize = 1;

    btnOption->setVisible(false);

    trayMenu = new QMenu(this);
    trayMenu->addAction(tr("Expand"), this, &MainForm::menuExpandClicked);
    trayMenu->addSeparator();
    trayMenu->addAction(tr("RTK&PLOT"), this, &MainForm::btnPlotClicked);
    trayMenu->addAction(tr("&RTKPOST"), this, &MainForm::btnPostClicked);
    trayMenu->addAction(tr("RTK&NAVI"), this, &MainForm::btnNaviClicked);
    trayMenu->addAction(tr("RTK&CONV"), this, &MainForm::btnConvClicked);
    trayMenu->addAction(tr("RTK&GET"), this, &MainForm::btnGetClicked);
    trayMenu->addAction(tr("RTK&STR"), this, &MainForm::btnStrClicked);
    trayMenu->addAction(tr("&NTRIP BROWSER"), this, &MainForm::btnNtripClicked);
    trayMenu->addSeparator();
    trayMenu->addAction(tr("&Exit"), this, &MainForm::close);

    trayIcon.setContextMenu(trayMenu);
    trayIcon.setIcon(QIcon(":/icons/rtk9"));
    trayIcon.setToolTip(windowTitle());

    QMenu *Popup = new QMenu();
    Popup->addAction(tr("&Expand"), this, &MainForm::menuExpandClicked);
    Popup->addSeparator();
    Popup->addAction(actionRtkConv);
    Popup->addAction(actionRtkGet);
    Popup->addAction(actionRtkNavi);
    Popup->addAction(actionRtkNtrip);
    Popup->addAction(actionRtkPlot);
    Popup->addAction(actionRtkPost);
    Popup->addAction(actionRtkStr);
    Popup->addAction(actionRtkVideo);
    Popup->addSeparator();
    Popup->addAction(tr("E&xit"),this, &MainForm::accept);
    btnRtklib->setMenu(Popup);

    connect(btnPlot, &QPushButton::clicked, this, &MainForm::btnPlotClicked);
    connect(btnConv, &QPushButton::clicked, this, &MainForm::btnConvClicked);
    connect(btnStr, &QPushButton::clicked, this, &MainForm::btnStrClicked);
    connect(btnPost, &QPushButton::clicked, this, &MainForm::btnPostClicked);
    connect(btnNtrip, &QPushButton::clicked, this, &MainForm::btnNtripClicked);
    connect(btnNavi, &QPushButton::clicked, this, &MainForm::btnNaviClicked);
    connect(btnTray, &QPushButton::clicked, this, &MainForm::btnTrayClicked);
    connect(btnGet, &QPushButton::clicked, this, &MainForm::btnGetClicked);
    connect(btnOption, &QPushButton::clicked, this, &MainForm::btnOptionClicked);
    connect(btnExit, &QPushButton::clicked, this, &MainForm::accept);
    connect(&trayIcon, &QSystemTrayIcon::activated, this, &MainForm::trayIconActivated);

    connect(actionRtkConv, &QAction::triggered, this, &MainForm::btnConvClicked);
    connect(actionRtkGet, &QAction::triggered, this, &MainForm::btnGetClicked);
    connect(actionRtkNavi, &QAction::triggered, this, &MainForm::btnNaviClicked);
    connect(actionRtkNtrip, &QAction::triggered, this, &MainForm::btnNtripClicked);
    connect(actionRtkPlot, &QAction::triggered, this, &MainForm::btnPlotClicked);
    connect(actionRtkPost, &QAction::triggered, this, &MainForm::btnPostClicked);
    connect(actionRtkStr, &QAction::triggered, this, &MainForm::btnStrClicked);

    updatePanel();
}
//---------------------------------------------------------------------------
void MainForm::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    QSettings settings(iniFile, QSettings::IniFormat);

    move(settings.value("pos/left", 0).toInt(),
         settings.value("pos/top", 0).toInt());

    resize(settings.value("pos/width", 310).toInt(),
           settings.value("pos/height", 79).toInt());

}
//---------------------------------------------------------------------------
void MainForm::closeEvent(QCloseEvent *event)
{
    if (event->spontaneous()) return;

    QSettings settings(iniFile, QSettings::IniFormat);
    settings.setValue("pos/left", pos().x());
    settings.setValue("pos/top", pos().y());
    settings.setValue("pos/width", width());
    settings.setValue("pos/height", height());
    settings.setValue("pos/option", option);
    settings.setValue("pos/minimize", minimize);
}
//---------------------------------------------------------------------------
void MainForm::btnPlotClicked()
{
    QString cmd1 = "./rtkplot_qt", cmd2 = "../rtkplot_qt/rtkplot_qt";
    QStringList opts;

    if (!execCommand(cmd1, opts)) execCommand(cmd2, opts);
}
//---------------------------------------------------------------------------
void MainForm::btnConvClicked()
{
    QString cmd1 = "./rtkconv_qt", cmd2 = "../rtkconv_qt/rtkconv_qt";
    QStringList opts;

    if (!execCommand(cmd1, opts)) execCommand(cmd2, opts);
}
//---------------------------------------------------------------------------
void MainForm::btnStrClicked()
{
    QString cmd1 = "./strsvr_qt", cmd2 = "../strsvr_qt/strsvr_qt";
    QStringList opts;

    if (!execCommand(cmd1, opts)) execCommand(cmd2, opts);
}
//---------------------------------------------------------------------------
void MainForm::btnPostClicked()
{
    QString cmd1 = "./rtkpost_qt", cmd2 = "../rtkpost_qt/rtkpost_qt";
    QStringList opts;

    if (!execCommand(cmd1, opts)) execCommand(cmd2, opts);
}
//---------------------------------------------------------------------------
void MainForm::btnNtripClicked()
{
    QString cmd1 = "./srctblbrows_qt", cmd2 = "../srctblbrows_qt/srctblbrows_qt";
    QStringList opts;

    if (!execCommand(cmd1, opts)) execCommand(cmd2, opts);
}
//---------------------------------------------------------------------------
void MainForm::btnNaviClicked()
{
    QString cmd1 = "./rtknavi_qt", cmd2 = "../rtknavi_qt/rtknavi_qt";
    QStringList opts;

    if (!execCommand(cmd1, opts)) execCommand(cmd2, opts);
}
//---------------------------------------------------------------------------
void MainForm::btnGetClicked()
{
    QString cmd1 = "./rtkget_qt", cmd2 = "../rtkget_qt/rtkget_qt";
    QStringList opts;

    if (!execCommand(cmd1, opts)) execCommand(cmd2, opts);
}
//---------------------------------------------------------------------------
int MainForm::execCommand(const QString &cmd, const QStringList &opt)
{
    return QProcess::startDetached(cmd, opt);
}
//---------------------------------------------------------------------------
void MainForm::btnTrayClicked()
{
    setVisible(false);
    trayIcon.setVisible(true);
}
//---------------------------------------------------------------------------
void MainForm::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::DoubleClick && reason != QSystemTrayIcon::Trigger) return;

    setVisible(true);
    trayIcon.setVisible(false);
}
//---------------------------------------------------------------------------
void MainForm::menuExpandClicked()
{
    setVisible(true);
    trayIcon.setVisible(false);
    minimize = 0;

    updatePanel();
}
//---------------------------------------------------------------------------
void MainForm::updatePanel(void)
{
    if (minimize) {
        panelTop->setVisible(false);
        panelBottom->setVisible(true);
    }
    else {
        panelTop->setVisible(true);
        panelBottom->setVisible(false);
    }
}
//---------------------------------------------------------------------------
void MainForm::btnOptionClicked()
{
    launchOptDlg->exec();
    if (launchOptDlg->result()!=QDialog::Accepted) return;

    updatePanel();
}
