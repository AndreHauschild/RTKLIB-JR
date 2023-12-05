//---------------------------------------------------------------------------
// rtkpost_qt : post-processing analysis
//
//          Copyright (C) 2007-2020 by T.TAKASU, All rights reserved.
//          ported to Qt by Jens Reimann
//
// options : rtkpost [-t title][-i file][-r file][-b file][-n file ...]
//                   [-d dir][-o file]
//                   [-ts y/m/d h:m:s][-te y/m/d h:m:s][-ti tint][-tu tunit]
//
//           -t title   window title
//           -i file    ini file path
//           -r file    rinex obs rover file
//           -b file    rinex obs base station file
//           -n file    rinex nav/clk, sp3, Bias-SINEX or ionex file
//           -d dir     output directory
//           -o file    output file
//           -ts y/m/d h:m:s time start
//           -te y/m/d h:m:s time end
//           -ti tint   time interval (s)
//           -tu tunit  time unit (hr)
//
// version : $Revision: 1.1 $ $Date: 2008/07/17 22:14:45 $
// history : 2008/07/14  1.0 new
//           2008/11/17  1.1 rtklib 2.1.1
//           2008/04/03  1.2 rtklib 2.3.1
//           2010/07/18  1.3 rtklib 2.4.0
//           2010/09/07  1.3 rtklib 2.4.1
//           2011/04/03  1.4 rtklib 2.4.2
//           2020/11/30  1.5 rtklib 2.4.3
//---------------------------------------------------------------------------
#include <clocale>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <QShowEvent>
#include <QCloseEvent>
#include <QCommandLineParser>
#include <QSettings>
#include <QFileDialog>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QtGlobal>
#include <QThread>
#include <QMimeData>
#include <QFileSystemModel>
#include <QCompleter>
#include <QRegularExpression>
#include <QTime>

#include "rtklib.h"
#include "postmain.h"
#include "postopt.h"
#include "kmzconv.h"
#include "timedlg.h"
#include "keydlg.h"
#include "aboutdlg.h"
#include "viewer.h"

#define PRGNAME     "RTKPOST-QT"
#define MAXHIST     20
#ifdef Q_OS_WIN
#define GOOGLE_EARTH "C:\\Program Files\\Google\\Google Earth Pro\\client\\googleearth.exe"
#endif
#ifdef Q_OS_LINUX
#define GOOGLE_EARTH "google-earth"
#endif
#ifndef GOOGLE_EARTH
#define GOOGLE_EARTH ""
#endif

// global variables ---------------------------------------------------------
static gtime_t tstart_ = {0, 0};         // time start for progress-bar
static gtime_t tend_   = {0, 0};         // time end for progress-bar

MainForm *mainForm;

extern "C" {

// show message in message area ---------------------------------------------
extern int showmsg(const char *format, ...)
{
    va_list arg;
    char buff[1024];
    if (*format) {
        va_start(arg,format);
        vsprintf(buff,format,arg);
        va_end(arg);
        QMetaObject::invokeMethod(mainForm, "showMessage",Qt::QueuedConnection,
                                  Q_ARG(QString, QString(buff)));
    }
    return mainForm->abortFlag;
}
// set time span of progress bar --------------------------------------------
extern void settspan(gtime_t ts, gtime_t te)
{
    tstart_ = ts;
    tend_   = te;
}
// set current time to show progress ----------------------------------------
extern void settime(gtime_t time)
{
    double tt;
    if (tend_.time != 0 && tstart_.time != 0 && (tt = timediff(tend_, tstart_)) > 0.0) {
        QMetaObject::invokeMethod(mainForm->pBProgress, "setValue",
                  Qt::QueuedConnection,
                  Q_ARG(int, (int)(timediff(time, tstart_) / tt * 100.0 + 0.5)));
    }
}

} // extern "C"

ProcessingThread::ProcessingThread(QObject *parent) : QThread(parent)
{
    n = stat = 0;
    prcopt = prcopt_default;
    solopt = solopt_default;
    ts.time = ts.sec = 0;
    te.time = te.sec = 0;
    ti = tu = 0;
    rov = base = QString();
    for (int i = 0; i < 6; i++) {infile[i] = new char[1024]; infile[i][0] = '\0';};
    outfile[0] = '\0';

    memset(&prcopt, 0, sizeof(prcopt_t));
    memset(&solopt, 0, sizeof(solopt_t));
    memset(&filopt, 0, sizeof(filopt_t));
}
ProcessingThread::~ProcessingThread()
{
    for (int i = 0; i < 6; i++) delete[] infile[i];
    rov = base = QString();
}
void ProcessingThread::addInput(const QString & file) {
    if (file != "") {
        strncpy(infile[n++], qPrintable(file), 1024);
    }
}
void ProcessingThread::run()
{
    if ((stat = postpos(ts, te, ti, tu, &prcopt, &solopt, &filopt, infile, n, outfile,
                        qPrintable(rov), qPrintable(base)))==1) {
        showmsg("aborted");
    };
    emit done(stat);
}

QString ProcessingThread::toList(const QString & list) {
    QString stations;
    QStringList lines = list.split("\n");

    foreach(QString line, lines)
    {
        int index = line.indexOf("#");
        if (index == -1) index = line.length();
        stations += line.mid(0, index) + " ";
    }
    return stations;
}

// constructor --------------------------------------------------------------
MainForm::MainForm(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    mainForm=  this;

    setWindowIcon(QIcon(":/icons/rktpost_Icon.ico"));
    setlocale(LC_NUMERIC, "C");
    int i;

    QString file = QApplication::applicationFilePath();
    QFileInfo fi(file);
    iniFile = fi.absolutePath() + "/" + fi.baseName() + ".ini";

    dynamicModel = ionosphereOption = troposphereOption = roverAntennaPcv = referenceAntennaPcv = ambiguityResolutionGPS = 0;
    roverPositionType = referencePositionType = 0;
    outputCntResetAmbiguity = 5;
    LockCntFixAmbiguity = 5;
    fixCntHoldAmbiguity = 10;
    maxAgeDiff = 30.0;
    rejectPhase = 30.0;
    rejectCode = 30.0;
    measurementErrorR1 = measurementErrorR2 = measurementErrorR5 = 100.0;
    measurementError2 = 0.004;
    measurementError3 = 0.003;
    measurementError4 = 1.0;
    satelliteClockStability = 1E-11;
    validThresAR = 3.0;
    roverAntennaE = roverAntennaN = roverAntennaU = referenceAntennaE = referenceAntennaN = referenceAntennaU = 0.0;

    for (i = 0; i < 3; i++) roverPosition[i] = 0.0;
    for (i = 0; i < 3; i++) referencePosition[i] = 0.0;

    pBProgress->setVisible(false);
    setAcceptDrops(true);

    optDialog = new OptDialog(this);
    convDialog = new ConvDialog(this);
    textViewer = new TextViewer(this);

    QCompleter *fileCompleter = new QCompleter(this);
    QFileSystemModel *fileModel = new QFileSystemModel(fileCompleter);
    fileModel->setRootPath("");
    fileCompleter->setModel(fileModel);
    cBInputFile1->setCompleter(fileCompleter);
    cBInputFile2->setCompleter(fileCompleter);
    cBInputFile3->setCompleter(fileCompleter);
    cBInputFile4->setCompleter(fileCompleter);
    cBInputFile5->setCompleter(fileCompleter);
    cBInputFile6->setCompleter(fileCompleter);
    cBOutputFile->setCompleter(fileCompleter);

    QCompleter *dirCompleter = new QCompleter(this);
    QFileSystemModel *dirModel = new QFileSystemModel(dirCompleter);
    dirModel->setRootPath("");
    dirModel->setFilter(QDir::AllDirs|QDir::Drives|QDir::NoDotAndDotDot);
    dirCompleter->setModel(dirModel);
    lEOutputDirectory->setCompleter(dirCompleter);

    btnAbort->setVisible(false);

    connect(btnPlot, &QPushButton::clicked, this, &MainForm::btnPlotClicked);
    connect(btnView, &QPushButton::clicked, this, &MainForm::btnViewClicked);
    connect(btnToKML, &QPushButton::clicked, this, &MainForm::btnToKMLClicked);
    connect(btnOption, &QPushButton::clicked, this, &MainForm::btnOptionClicked);
    connect(btnExec, &QPushButton::clicked, this, &MainForm::btnExecClicked);
    connect(btnAbort, &QPushButton::clicked, this, &MainForm::btnAbortClicked);
    connect(btnExit, &QPushButton::clicked, this, &MainForm::close);
    connect(btnAbout, &QPushButton::clicked, this, &MainForm::btnAboutClicked);
    connect(btnTimeStart, &QPushButton::clicked, this, &MainForm::btnTimeStartClicked);
    connect(btnTimeStop, &QPushButton::clicked, this, &MainForm::btnTimeStopClicked);
    connect(btnInputFile1, &QPushButton::clicked, this, &MainForm::btnInputFile1Clicked);
    connect(btnInputFile2, &QPushButton::clicked, this, &MainForm::btnInputFile2Clicked);
    connect(btnInputFile3, &QPushButton::clicked, this, &MainForm::btnInputFile3Clicked);
    connect(btnInputFile4, &QPushButton::clicked, this, &MainForm::btnInputFile4Clicked);
    connect(btnInputFile5, &QPushButton::clicked, this, &MainForm::btnInputFile5Clicked);
    connect(btnInputFile6, &QPushButton::clicked, this, &MainForm::btnInputFile6Clicked);
    connect(btnOutputFile, &QPushButton::clicked, this, &MainForm::btnOutputFileClicked);
    connect(btnInputView1, &QPushButton::clicked, this, &MainForm::btnInputView1Clicked);
    connect(btnInputView2, &QPushButton::clicked, this, &MainForm::btnInputView2Clicked);
    connect(btnInputView3, &QPushButton::clicked, this, &MainForm::btnInputView3Clicked);
    connect(btnInputView4, &QPushButton::clicked, this, &MainForm::btnInputView4Clicked);
    connect(btnInputView5, &QPushButton::clicked, this, &MainForm::btnInputView5Clicked);
    connect(btnInputView6, &QPushButton::clicked, this, &MainForm::btnInputView6Clicked);
    connect(btnOutputView1, &QPushButton::clicked, this, &MainForm::btnOutputView1Clicked);
    connect(btnOutputView2, &QPushButton::clicked, this, &MainForm::btnOutputView2Clicked);
    connect(btnInputPlot1, &QPushButton::clicked, this, &MainForm::btnInputPlot1Clicked);
    connect(btnInputPlot2, &QPushButton::clicked, this, &MainForm::btnInputPlot2Clicked);
    connect(btnKeyword, &QPushButton::clicked, this, &MainForm::btnKeywordClicked);
    connect(cBTimeStart, &QCheckBox::clicked, this, &MainForm::updateEnable);
    connect(cBTimeEnd, &QCheckBox::clicked, this, &MainForm::updateEnable);
    connect(cBTimeIntervalF, &QCheckBox::clicked, this, &MainForm::updateEnable);
    connect(cBTimeUnitF, &QCheckBox::clicked, this, &MainForm::updateEnable);
    connect(cBInputFile1, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainForm::setOutputFile);
    connect(cBOutputDirectoryEnable, &QCheckBox::clicked, this, &MainForm::outputDirectoryEnableClicked);
    connect(lEOutputDirectory, &QLineEdit::editingFinished, this, &MainForm::setOutputFile);
    connect(btnOutputDirectory, &QPushButton::clicked, this, &MainForm::btnOutputDirectoryClicked);

    setWindowTitle(QString("%1 ver.%2 %3").arg(PRGNAME).arg(VER_RTKLIB).arg(PATCH_LEVEL));
}
// callback on form show ----------------------------------------------------
void MainForm::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) return;

    QComboBox *ifile[] = {cBInputFile3, cBInputFile4, cBInputFile5, cBInputFile6};
    int inputflag = 0;

    QCommandLineParser parser;
    parser.setApplicationDescription("RTK post");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    QCommandLineOption iniFileOption(QStringList() << "i",
            QCoreApplication::translate("main", "use init file <file>"),
            QCoreApplication::translate("main", "ini file"));
    parser.addOption(iniFileOption);

    QCommandLineOption titleOption(QStringList() << "t",
            QCoreApplication::translate("main", "use window tile <title>"),
            QCoreApplication::translate("main", "title"));
    parser.addOption(titleOption);

    QCommandLineOption roverOption(QStringList() << "r",
            QCoreApplication::translate("main", "rinex obs rover <file>"),
            QCoreApplication::translate("main", "file"));
    parser.addOption(roverOption);

    QCommandLineOption baseStationOption(QStringList() << "b",
            QCoreApplication::translate("main", "rinex obs base station <path>"),
            QCoreApplication::translate("main", "file"));
    parser.addOption(baseStationOption);

    QCommandLineOption navFileOption(QStringList() << "n" << "file",
            QCoreApplication::translate("main", "rinex nav/clk, sp3, ionex or sp3 <file>"),
            QCoreApplication::translate("main", "file"));
    parser.addOption(navFileOption);

    QCommandLineOption outputOption(QStringList() << "o",
            QCoreApplication::translate("main", "output file <file>"),
            QCoreApplication::translate("main", "file"));
    parser.addOption(outputOption);

    QCommandLineOption outputDirOption(QStringList() << "d",
            QCoreApplication::translate("main", "output directory <dir>"),
            QCoreApplication::translate("main", "dir"));
    parser.addOption(outputDirOption);

    QCommandLineOption timeStartOption(QStringList() << "ts",
            QCoreApplication::translate("main", "time start"),
            QCoreApplication::translate("main", "yyyy/mm/dd hh:mm:ss"));
    parser.addOption(timeStartOption);

    QCommandLineOption timeEndOption(QStringList() << "te",
            QCoreApplication::translate("main", "time end"),
            QCoreApplication::translate("main", "yyyy/mm/dd hh:mm:ss"));
    parser.addOption(timeEndOption);

    QCommandLineOption timeIntervalOption(QStringList() << "ti",
            QCoreApplication::translate("main", "time interval (s)"),
            QCoreApplication::translate("main", "time"));
    parser.addOption(timeIntervalOption);

    QCommandLineOption timeUnitOption(QStringList() << "tu",
            QCoreApplication::translate("main", "time unit (hr)"),
            QCoreApplication::translate("main", "unit"));
    parser.addOption(timeUnitOption);

    parser.process(*QApplication::instance());

    if (parser.isSet(iniFileOption)) {
        iniFile=parser.value(iniFileOption);
    }

    loadOptions();

    if (parser.isSet(titleOption)) {
        setWindowTitle(parser.value(titleOption));
    }
    if (parser.isSet(roverOption)) {
        cBInputFile1->setCurrentText(parser.value(roverOption));
        inputflag = 1;
    };
    if (parser.isSet(baseStationOption)) {
        cBInputFile2->setCurrentText(parser.value(baseStationOption));
    }
    if (parser.isSet(navFileOption)) {
        QStringList files = parser.values(navFileOption);
        for (int n = 0; n < files.size() && n < 4; n++)
            ifile[n]->setCurrentText(files.at(n));
    }
    if (parser.isSet(outputOption)) {
        cBOutputFile->setCurrentText(parser.value(outputOption));
    }
    if (parser.isSet(outputDirOption)) {
        cBOutputDirectoryEnable->setChecked(true);
        lEOutputDirectory->setText(parser.value(outputDirOption));
    }
    if (parser.isSet(timeStartOption)) {
        cBTimeStart->setChecked(true);
        dtDateTimeStart->setDateTime(QDateTime::fromString(parser.value(timeStartOption), "yyyy/MM/dd hh:mm:ss"));
    }
    if (parser.isSet(timeEndOption)) {
        cBTimeEnd->setChecked(true);
        dtDateTimeStop->setDateTime(QDateTime::fromString(parser.value(timeEndOption), "yyyy/MM/dd hh:mm:ss"));
    }
    if (parser.isSet(timeIntervalOption)) {
        cBTimeIntervalF->setChecked(true);
        cBTimeInterval->setCurrentText(parser.value(timeIntervalOption));
    }
    if (parser.isSet(timeUnitOption)) {
        cBTimeUnitF->setChecked(true);
        lETimeUnit->setText(parser.value(timeUnitOption));
    }

    if (inputflag) setOutputFile();

    updateEnable();
}
// callback on form close ---------------------------------------------------
void MainForm::closeEvent(QCloseEvent *)
{
    saveOptions();
}
// callback on drop files ---------------------------------------------------
void  MainForm::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}
void  MainForm::dropEvent(QDropEvent *event)
{
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    QPointF point = event->position();
#else
    QPointF point = event->pos();
#endif
    int top;

    if (!event->mimeData()->hasFormat("text/uri-list")) return;

    QString file = QDir::toNativeSeparators(event->mimeData()->text());

    top = Panel1->pos().y() + Panel4->pos().y();
    if (point.y() <= top + cBInputFile1->pos().y() + cBInputFile1->height()) {
        cBInputFile1->setCurrentText(file);
        setOutputFile();
    }
    else if (point.y() <= top + cBInputFile2->pos().y() + cBInputFile2->height()) {
        cBInputFile2->setCurrentText(file);
    }
    else if (point.y() <= top + cBInputFile3->pos().y() + cBInputFile3->height()) {
        cBInputFile3->setCurrentText(file);
    }
    else if (point.y() <= top + cBInputFile4->pos().y() + cBInputFile4->height()) {
        cBInputFile4->setCurrentText(file);
    }
    else if (point.y() <= top + cBInputFile5->pos().y() + cBInputFile5->height()) {
        cBInputFile5->setCurrentText(file);
    }
    else if (point.y() <= top + cBInputFile6->pos().y() + cBInputFile6->height()) {
        cBInputFile6->setCurrentText(file);
    }
}
// callback on button-plot --------------------------------------------------
void MainForm::btnPlotClicked()
{
    QString OutputFile_Text = cBOutputFile->currentText();
    QString file = filePath(OutputFile_Text);
    QString cmd1 = "rtkplot_qt", cmd2 = "../rtkplot_qt/rtkplot_qt", cmd3 = "../../../bin/rtkplot_qt";
    QStringList opts;

    opts += file;

    if (!execCommand(cmd1, opts, 1) && !execCommand(cmd2, opts, 1) && !execCommand(cmd3, opts, 1)) {
        showMessage("error : rtkplot_qt execution");
    }
}
// callback on button-view --------------------------------------------------
void MainForm::btnViewClicked()
{
    QString OutputFile_Text = cBOutputFile->currentText();
    viewFile(filePath(OutputFile_Text));
}
// callback on button-to-kml ------------------------------------------------
void MainForm::btnToKMLClicked()
{
    QString OutputFile_Text = cBOutputFile->currentText();
    convDialog->setInput(filePath(OutputFile_Text));
    convDialog->exec();

}
// callback on button-options -----------------------------------------------
void MainForm::btnOptionClicked()
{
    int format = solutionFormat;
    optDialog->exec();
    if (optDialog->result() != QDialog::Accepted) return;
    if ((format == SOLF_NMEA) != (solutionFormat == SOLF_NMEA)) {
        setOutputFile();
    }
    updateEnable();
}
// callback on button-execute -----------------------------------------------
void MainForm::btnExecClicked()
{
    QString OutputFile_Text = cBOutputFile->currentText();
    abortFlag = false;

    if (cBInputFile1->currentText() == "") {
        showmsg("error : no rinex obs file (rover)");
        return;
    }
    if (cBInputFile2->currentText() == "" && PMODE_DGPS <= positionMode && positionMode <= PMODE_FIXED) {
        showmsg("error : no rinex obs file (base station)");
        return;
    }
    if (cBOutputFile->currentText() == "") {
        showmsg("error : no output file");
        return;
    }
    QFileInfo f = QFileInfo(OutputFile_Text);
    if (f.suffix().contains(".obs",Qt::CaseInsensitive)||
        f.suffix().contains(".rnx",Qt::CaseInsensitive)||
        f.suffix().contains(".nav",Qt::CaseInsensitive)||
        f.suffix().contains(".gnav",Qt::CaseInsensitive)||
        f.suffix().contains(".gz",Qt::CaseInsensitive)||
        f.suffix().contains(".Z",Qt::CaseInsensitive)||
        f.suffix().contains(QRegularExpression(R"(\.\d\d[ondg])", QRegularExpression::CaseInsensitiveOption))) {
        showmsg("error : invalid extension of output file (%s)", qPrintable(OutputFile_Text));
        return;
    }
    showmsg("");

    btnAbort ->setVisible(true);
    btnExec->setVisible(false);
    btnExit->setEnabled(false);
    btnView->setEnabled(false);
    btnToKML->setEnabled(false);
    btnPlot->setEnabled(false);
    btnOption->setEnabled(false);
    Panel1->setEnabled(false);

    execProcessing();
}
// callback on processing finished-------------------------------------------
void MainForm::processingFinished(int stat)
{
    setCursor(Qt::ArrowCursor);
    pBProgress->setVisible(false);

    if (stat >= 0) {
        addHistory(cBInputFile1);
        addHistory(cBInputFile2);
        addHistory(cBInputFile3);
        addHistory(cBInputFile4);
        addHistory(cBInputFile5);
        addHistory(cBInputFile6);
        addHistory(cBOutputFile);
    }

    if (lblMessage->text().contains("processing")) {
        showmsg("done");
    }
    btnAbort ->setVisible(false);
    btnExec->setVisible(true);
    btnExec->setEnabled(true);
    btnExit->setEnabled(true);
    btnView->setEnabled(true);
    btnToKML->setEnabled(true);
    btnPlot->setEnabled(true);
    btnOption->setEnabled(true);
    Panel1->setEnabled(true);

    delete processingThread;
}
// callback on button-abort -------------------------------------------------
void MainForm::btnAbortClicked()
{
    abortFlag = true;
    showmsg("aborted");
}
// callback on button-about -------------------------------------------------
void MainForm::btnAboutClicked()
{
    QString prog = PRGNAME;
    AboutDialog *aboutDialog = new AboutDialog(this);

    aboutDialog->aboutString = prog;
    aboutDialog->iconIndex = 1;
    aboutDialog->exec();

    delete aboutDialog;
}
// callback on button-time-1 ------------------------------------------------
void MainForm::btnTimeStartClicked()
{
    TimeDialog *timeDialog = new TimeDialog(this);
    timeDialog->time = getTimeStart();
    timeDialog->exec();

    delete timeDialog;
}
// callback on button-time-2 ------------------------------------------------
void MainForm::btnTimeStopClicked()
{
    TimeDialog *timeDialog = new TimeDialog(this);

    timeDialog->time = getTimeStop();
    timeDialog->exec();

    delete timeDialog;
}
// callback on button-inputfile-1 -------------------------------------------
void MainForm::btnInputFile1Clicked()
{
    cBInputFile1->setCurrentText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("RINEX OBS (Rover) File"), cBInputFile1->currentText(), tr("All (*.*);;RINEX OBS (*.rnx *.obs *.*O *.*D)"))));
    setOutputFile();
}
// callback on button-inputfile-2 ------------------------------------------
void MainForm::btnInputFile2Clicked()
{
    cBInputFile2->setCurrentText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("RINEX OBS (Base Station) File"), cBInputFile2->currentText(), tr("All (*.*);;RINEX OBS (*.rnx *.obs *.*O *.*D)"))));
}
// callback on button-inputfile-3 -------------------------------------------
void MainForm::btnInputFile3Clicked()
{
    cBInputFile3->setCurrentText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("RINEX NAV/CLK,SP3,Bias-SINEX,IONEX or SBAS/EMS File"), cBInputFile3->currentText(), tr("All (*.*);;RINEX NAV (*.rnx *.*nav *.*N *.*P *.*G *.*H *.*Q)"))));
}
// callback on button-inputfile-4 -------------------------------------------
void MainForm::btnInputFile4Clicked()
{
    cBInputFile4->setCurrentText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("RINEX NAV/CLK,SP3,Bias-SINEX,IONEX or SBAS/EMS File"), cBInputFile4->currentText(), tr("All (*.*);;Precise Ephemeris/Clock/Biases (*.SP3 *.sp3 *.eph* *.CLK *.clk* *.BIA)"))));
}
// callback on button-inputfile-5 -------------------------------------------
void MainForm::btnInputFile5Clicked()
{
    cBInputFile5->setCurrentText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("RINEX NAV/CLK,SP3,Bias-SINEX,IONEX or SBAS/EMS File"), cBInputFile5->currentText(), tr("All (*.*);;Precise Ephemeris/Clock/Biases (*.SP3 *.sp3 *.eph* *.CLK *.clk* *.BIA)"))));
}
// callback on button-inputfile-6 -------------------------------------------
void MainForm::btnInputFile6Clicked()
{
    cBInputFile6->setCurrentText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("RINEX NAV/CLK,SP3,Bias-SINEX,IONEX or SBAS/EMS File"), cBInputFile6->currentText(), tr("All (*.*);;Bias-SINEX (*.BIA *.BSX),IONEX (*.*i *.ionex),SBAS (*.sbs *.ems)"))));
}
// callback on button-outputfile --------------------------------------------
void MainForm::btnOutputFileClicked()
{
    cBOutputFile->setCurrentText(QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Output File"), cBOutputFile->currentText(), tr("All (*.*);;Position Files (*.pos)"))));
}
// callback on button-inputview-1 -------------------------------------------
void MainForm::btnInputView1Clicked()
{
    QString InputFile1_Text = cBInputFile1->currentText();
    viewFile(filePath(InputFile1_Text));
}
// callback on button-inputview-2 -------------------------------------------
void MainForm::btnInputView2Clicked()
{
    QString InputFile2_Text = cBInputFile2->currentText();
    viewFile(filePath(InputFile2_Text));
}
// callback on button-inputview-3 -------------------------------------------
void MainForm::btnInputView3Clicked()
{
    QString InputFile1_Text = cBInputFile1->currentText();
    QString InputFile3_Text = cBInputFile3->currentText();
    QString file = filePath(InputFile3_Text);
    QString f;

    if (file == "") {
        file = filePath(InputFile1_Text);
        if (!obsToNav(file, f)) return;
        file = f;
    }
    viewFile(file);
}
// callback on button-inputview-4 -------------------------------------------
void MainForm::btnInputView4Clicked()
{
    QString InputFile4_Text = cBInputFile4->currentText();
    viewFile(filePath(InputFile4_Text));
}
// callback on button-inputview-5 -------------------------------------------
void MainForm::btnInputView5Clicked()
{
    QString InputFile5_Text = cBInputFile5->currentText();
    viewFile(filePath(InputFile5_Text));
}
// callback on button-inputview-6 -------------------------------------------
void MainForm::btnInputView6Clicked()
{
    QString InputFile6_Text = cBInputFile6->currentText();
    viewFile(filePath(InputFile6_Text));
}
// callback on button-outputview-1 ------------------------------------------
void MainForm::btnOutputView1Clicked()
{
    QString OutputFile_Text = cBOutputFile->currentText();
    QString file = filePath(OutputFile_Text) + ".stat";
    if (!QFile::exists(file)) return;
    viewFile(file);
}
// callback on button-outputview-2 ------------------------------------------
void MainForm::btnOutputView2Clicked()
{
    QString OutputFile_Text=cBOutputFile->currentText();
    QString file=filePath(OutputFile_Text)+".trace";
    if (!QFile::exists(file)) return;
    viewFile(file);
}
// callback on button-inputplot-1 -------------------------------------------
void MainForm::btnInputPlot1Clicked()
{
    QString InputFile1_Text = cBInputFile1->currentText();
    QString InputFile2_Text = cBInputFile2->currentText();
    QString InputFile3_Text = cBInputFile3->currentText();
    QString InputFile4_Text = cBInputFile4->currentText();
    QString InputFile5_Text = cBInputFile5->currentText();
    QString InputFile6_Text = cBInputFile6->currentText();
    QString files[6];
    QString cmd1 = "rtkplot_qt", cmd2= "../rtkplot_qt/rtkplot_qt", cmd3 = "../../../bin/rtkplot_qt";
    QStringList opts;
    QString navfile;

    files[0] = filePath(InputFile1_Text); /* obs rover */
    files[1] = filePath(InputFile2_Text); /* obs base */
    files[2] = filePath(InputFile3_Text);
    files[3] = filePath(InputFile4_Text);
    files[4] = filePath(InputFile5_Text);
    files[5] = filePath(InputFile6_Text);

    if (files[2] == "") {
        if (obsToNav(files[0], navfile)) files[2] = navfile;
    }
    opts << "-r" << files[0] << files[2] << files[3] << files[4] << files[5];

    if (!execCommand(cmd1, opts,1) && !execCommand(cmd2, opts,1) && !execCommand(cmd3, opts,1)) {
        showMessage("error : rtkplot_qt execution");
    }
}
// callback on button-inputplot-2 -------------------------------------------
void MainForm::btnInputPlot2Clicked()
{
    QString InputFile1_Text = cBInputFile1->currentText();
    QString InputFile2_Text = cBInputFile2->currentText();
    QString InputFile3_Text = cBInputFile3->currentText();
    QString InputFile4_Text = cBInputFile4->currentText();
    QString InputFile5_Text = cBInputFile5->currentText();
    QString InputFile6_Text = cBInputFile6->currentText();
    QString files[6];
    QString cmd1 = "rtkplot_qt", cmd2= "../rtkplot_qt/rtkplot_qt", cmd3 = "../../../bin/rtkplot_qt";
    QStringList opts;
    QString navfile;

    files[0] = filePath(InputFile1_Text); /* obs rover */
    files[1] = filePath(InputFile2_Text); /* obs base */
    files[2] = filePath(InputFile3_Text);
    files[3] = filePath(InputFile4_Text);
    files[4] = filePath(InputFile5_Text);
    files[5] = filePath(InputFile6_Text);

    if (files[2] == "") {
        if (obsToNav(files[0], navfile)) files[2]=navfile;
    }
    opts << "-r" << files[1] << files[2] << files[3] << files[4] << files[5];

    if (!execCommand(cmd1, opts,1) && !execCommand(cmd2, opts,1) && !execCommand(cmd3, opts,1)) {
        showMessage("error : rtkplot_qt execution");
    }
}
// callback on button-output-directory --------------------------------------
void MainForm::btnOutputDirectoryClicked()
{
    lEOutputDirectory->setText(QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Output Directory"), lEOutputDirectory->text())));
}
// callback on button keyword -----------------------------------------------
void MainForm::btnKeywordClicked()
{
    KeyDialog *keyDialog = new KeyDialog(this);
    keyDialog->flag = 2;
    keyDialog->exec();

    delete keyDialog;
}
// callback on output-directory checked -------------------------------------
void MainForm::outputDirectoryEnableClicked()
{
    updateEnable();
    setOutputFile();
}
// set output file path -----------------------------------------------------
void MainForm::setOutputFile()
{
    QString InputFile1_Text = cBInputFile1->currentText();
    QString OutDir_Text = lEOutputDirectory->text();
    QString ofile, ifile;

    if (cBInputFile1->currentText() == "") return;

    if (cBOutputFile->currentText() == "") {
      ifile = InputFile1_Text;
      if (cBOutputDirectoryEnable->isChecked()) {
          QFileInfo f(ifile);
          ofile = OutDir_Text + "/" + f.baseName();
      }
      else {
          QFileInfo f(ifile);
          ofile = f.absolutePath() + "/" + f.baseName();
      }
      ofile += solutionFormat == SOLF_NMEA ? ".nmea" : ".pos";
      ofile.replace('*', '0');
    }
    else {
      ofile = cBOutputFile->currentText();
    };
    cBOutputFile->setCurrentText(QDir::toNativeSeparators(ofile));

}
// execute post-processing --------------------------------------------------
void MainForm::execProcessing()
{
    QString InputFile1_Text = cBInputFile1->currentText(), InputFile2_Text = cBInputFile2->currentText();
    QString InputFile3_Text = cBInputFile3->currentText(), InputFile4_Text = cBInputFile4->currentText();
    QString InputFile5_Text = cBInputFile5->currentText(), InputFile6_Text = cBInputFile6->currentText();
    QString OutputFile_Text = cBOutputFile->currentText();
    QString temp;

    processingThread = new ProcessingThread(this);

    // get processing options
    if (cBTimeStart->isChecked()) processingThread->ts = getTimeStart();
    if (cBTimeEnd->isChecked()) processingThread->te = getTimeStop();
    if (cBTimeIntervalF ->isChecked()) processingThread->ti = cBTimeInterval->currentText().toDouble();
    if (cBTimeUnitF->isChecked()) processingThread->tu = lETimeUnit->text().toDouble() * 3600.0;

    processingThread->prcopt = prcopt_default;
    if (!getOption(processingThread->prcopt, processingThread->solopt, processingThread->filopt)) {
        processingFinished(0);
        return;
    }

    // set input/output files
    processingThread->addInput(InputFile1_Text);

    if (PMODE_DGPS <= processingThread->prcopt.mode && processingThread->prcopt.mode <= PMODE_FIXED) {
        processingThread->addInput(InputFile2_Text);
    }
    if (InputFile3_Text != "") {
        processingThread->addInput(InputFile3_Text);
    }
    else if (!obsToNav(InputFile1_Text, temp)) {
        showmsg("error: no navigation data");
        processingFinished(0);
        return;
    } else
        processingThread->addInput(temp);

    if (InputFile4_Text != "") {
        processingThread->addInput(InputFile4_Text);
    }
    if (InputFile5_Text != "") {
        processingThread->addInput(InputFile5_Text);
    }
    if (InputFile6_Text != "") {
        processingThread->addInput(InputFile6_Text);
    }
    strncpy(processingThread->outfile, qPrintable(OutputFile_Text), 1024);

    // confirm overwrite
    if (!cBTimeStart->isChecked() || !cBTimeEnd->isChecked()) {
        if (QFileInfo::exists(processingThread->outfile)) {
            if (QMessageBox::question(this, tr("Overwrite"), QString(tr("Overwrite existing file %1.")).arg(processingThread->outfile)) != QMessageBox::Yes) {
                processingFinished(0);
                return;
            }
        }
    }

    // set rover and base station list
    processingThread->rov = processingThread->toList(roverList);
    processingThread->base = processingThread->toList(baseList);

    pBProgress->setValue(0);
    pBProgress->setVisible(true);
    showmsg("reading...");

    setCursor(Qt::WaitCursor);

    // post processing positioning
    connect(processingThread, &ProcessingThread::done, this, &MainForm::processingFinished);

    processingThread->start();

    return;
}
// get processing and solution options --------------------------------------
int MainForm::getOption(prcopt_t &prcopt, solopt_t &solopt, filopt_t &filopt)
{
    char buff[1024],*p;
    int sat,ex;

    // processing options
    prcopt.mode = positionMode;
    prcopt.soltype = solution;
    prcopt.nf = frequencies + 1;
    prcopt.navsys = navigationSystems;
    prcopt.elmin = elevationMask * D2R;
    prcopt.snrmask = snrMask;
    prcopt.sateph = satelliteEphemeris;
    prcopt.modear = ambiguityResolutionGPS;
    prcopt.glomodear = ambiguityResolutionGLO;
    prcopt.bdsmodear = ambiguityResolutionBDS;
    prcopt.maxout = outputCntResetAmbiguity;
    prcopt.minfix = fixCntHoldAmbiguity;
    prcopt.minlock = LockCntFixAmbiguity;
    prcopt.ionoopt = ionosphereOption;
    prcopt.tropopt = troposphereOption;
    prcopt.posopt[0] = positionOption[0];
    prcopt.posopt[1] = positionOption[1];
    prcopt.posopt[2] = positionOption[2];
    prcopt.posopt[3] = positionOption[3];
    prcopt.posopt[4] = positionOption[4];
    prcopt.posopt[5] = positionOption[5];
    prcopt.dynamics = dynamicModel;
    prcopt.tidecorr = tideCorrection;
    prcopt.rcvBiasL5= receiverBiasEstimation;
    prcopt.armaxiter=  ARIter;
    prcopt.niter = numIter;
    prcopt.intpref = intpolateReferenceObs;
    prcopt.minfixsats = minFixSats;
    prcopt.minholdsats = minHoldSats;
    prcopt.mindropsats = minDropSats;
    prcopt.arfilter = ARFilter;
    prcopt.intpref  = intpolateReferenceObs;
    prcopt.sbassatsel = sbasSat;
    prcopt.eratio[0] = measurementErrorR1;
    prcopt.eratio[1] = measurementErrorR2;
    prcopt.eratio[2] = measurementErrorR5;
    prcopt.err[1] = measurementError2;
    prcopt.err[2] = measurementError3;
    prcopt.err[3] = measurementError4;
    prcopt.err[4] = measurementError5;
    prcopt.err[5] = measurementError6;
    prcopt.err[6] = measurementError7;
    prcopt.err[7] = measurementError8;
    prcopt.prn[0] = processNoise1;
    prcopt.prn[1] = processNoise2;
    prcopt.prn[2] = processNoise3;
    prcopt.prn[3] = processNoise4;
    prcopt.prn[4] = processNoise5;
    prcopt.sclkstab = satelliteClockStability;
    prcopt.thresar[0] = validThresAR;
    prcopt.thresar[1] = maxPositionVarAR;
    prcopt.thresar[2] = glonassHwBias;
    prcopt.thresar[3] = thresAR3;
    prcopt.thresar[4] = thresAR4;
    prcopt.thresar[5] = validThresARMin;
    prcopt.thresar[6] = validThresARMax;
    prcopt.elmaskar = elevationMaskAR * D2R;
    prcopt.elmaskhold = elevationMaskHold * D2R;
    prcopt.thresslip = slipThreshold;
    prcopt.thresdop = dopplerThreshold;
    prcopt.maxtdiff = maxAgeDiff;
    prcopt.maxinno[1] = rejectCode;
    prcopt.maxinno[0] = rejectPhase;
    prcopt.varholdamb = varHoldAmb;
    prcopt.gainholdamb = gainHoldAmb;
    prcopt.outsingle = outputSingle;
    if (baseLineConstrain) {
        prcopt.baseline[0] = baseLine[0];
        prcopt.baseline[1] = baseLine[1];
    }
    else {
        prcopt.baseline[0] = 0.0;
        prcopt.baseline[1] = 0.0;
    }
    if (positionMode != PMODE_FIXED && positionMode != PMODE_PPP_FIXED) {
        for (int i = 0; i < 3; i++) prcopt.ru[i] = 0.0;
    }
    else if (roverPositionType <= 2) {
        for (int i = 0; i < 3; i++) prcopt.ru[i] = roverPosition[i];
    }
    else prcopt.rovpos = roverPositionType - 2; /* 1:single,2:posfile,3:rinex */

    if (positionMode == PMODE_SINGLE || positionMode == PMODE_MOVEB) {
        for (int i = 0; i < 3; i++) prcopt.rb[i] = 0.0;
    }
    else if (referencePositionType<=2) {
        for (int i = 0; i < 3; i++) prcopt.rb[i] = referencePosition[i];
    }
    else prcopt.refpos = referencePositionType - 2;

    if (roverAntennaPcv) {
        strncpy(prcopt.anttype[0], qPrintable(roverAntenna), 64);
        prcopt.antdel[0][0] = roverAntennaE;
        prcopt.antdel[0][1] = roverAntennaN;
        prcopt.antdel[0][2] = roverAntennaU;
    }
    if (referenceAntennaPcv) {
        strncpy(prcopt.anttype[1], qPrintable(referenceAntenna), 64);
        prcopt.antdel[1][0] = referenceAntennaE;
        prcopt.antdel[1][1] = referenceAntennaN;
        prcopt.antdel[1][2] = referenceAntennaU;
    }
    if (excludedSatellites != "") { // excluded satellites
        strncpy(buff, qPrintable(excludedSatellites), 1024);
        for (p = strtok(buff, " "); p ; p = strtok(NULL, " ")) {
            if (*p == '+') {ex = 2; p++;} else ex = 1;
            if (!(sat = satid2no(p))) continue;
            prcopt.exsats[sat - 1] = ex;
        }
    }

    strncpy(prcopt.rnxopt[0], qPrintable(rnxOptions1), 256);
    strncpy(prcopt.rnxopt[1], qPrintable(rnxOptions2), 256);
    strncpy(prcopt.pppopt, qPrintable(pppOptions), 256);

    // solution options
    solopt.posf = solutionFormat;
    solopt.times = timeFormat == 0 ? 0 : timeFormat - 1;
    solopt.timef = timeFormat == 0 ? 0 : 1;
    solopt.timeu = timeDecimal <= 0 ? 0 : timeDecimal;
    solopt.degf = latLonFormat;
    solopt.outhead = outputHeader;
    solopt.outopt = outputOptions;
    solopt.outvel = outputVelocity;
    solopt.maxsolstd = maxSolutionStd;
    solopt.datum = outputDatum;
    solopt.height = outputHeight;
    solopt.geoid = outputGeoid;
    solopt.solstatic = solutionStatic;
    solopt.sstat = debugStatus;
    solopt.trace = debugTrace;
    strncpy(solopt.sep, fieldSeperator != "" ? qPrintable(fieldSeperator) : " ", 64);
    strncpy(solopt.prog, qPrintable(QString("%1 ver.%2 %3").arg(PRGNAME).arg(VER_RTKLIB).arg(PATCH_LEVEL)), 64);

    // file options
    strncpy(filopt.satantp, qPrintable(satellitePcvFile), 1024);
    strncpy(filopt.rcvantp, qPrintable(antennaPcvFile), 1024);
    strncpy(filopt.stapos, qPrintable(stationPositionFile), 1024);
    strncpy(filopt.geoid, qPrintable(geoidDataFile), 1024);
    strncpy(filopt.iono, qPrintable(ionosphereFile), 1024);
    strncpy(filopt.eop, qPrintable(eopFile), 1024);
    strncpy(filopt.dcb, qPrintable(dcbFile), 1024);
    strncpy(filopt.blq, qPrintable(blqFile), 1024);

    return 1;
}
// observation file to nav file ---------------------------------------------
int MainForm::obsToNav(const QString &obsfile, QString &navfile)
{
    int p;
    QFileInfo f(obsfile);
    navfile = f.canonicalPath() + f.completeBaseName();
    QString suffix = f.suffix();

    if (suffix == "") return 0;
    if ((suffix.length() == 3) && (suffix.at(2).toLower() == 'o')) suffix[2] = '*';
    else if ((suffix.length() == 3) && (suffix.at(2).toLower() == 'd')) suffix[2] = '*';
    else if (suffix.toLower() == "obs") suffix = "*nav";
    else if (((p = suffix.indexOf("gz")) != -1) || (( p = suffix.indexOf('Z')) != -1)) {
        if (p < 1) return 0;
        if (suffix.at(p - 1).toLower() == 'o') suffix[p - 1] = '*';
        else if (suffix.at(p - 1).toLower() == 'd') suffix[p - 1] = '*';
        else return 0;
    }
    else return 0;
    return 1;
}
// replace file path with keywords ------------------------------------------
QString MainForm::filePath(const QString &file)
{
    gtime_t ts = {0,0};
    int p;
    char rov[256] = "", base[256] = "", path[1024];

    if (cBTimeStart->isChecked()) ts = getTimeStart();

    p = 0;
    while ((p = roverList.indexOf("\n", p)) != -1){
        if ((p < roverList.length()) && (roverList.at(p) != '#')) break;
    }
    if (p != -1) strncpy(rov, qPrintable(roverList.mid(p)), 256);
    else strncpy(rov, qPrintable(roverList), 256);

    p = 0;
    while ((p = baseList.indexOf("\n", p)) != -1){
        if ((p < baseList.length()) && (baseList.at(p) != '#')) break;
    }
    if (p != -1) strncpy(base, qPrintable(baseList.mid(p)), 256);
    else strncpy(base, qPrintable(roverList), 256);

    reppath(qPrintable(file), path, ts, rov, base);

    return QString(path);
}
// read history -------------------------------------------------------------
void MainForm::readList(QComboBox* combo, QSettings *ini, const QString &key)
{
    QString item;
    int i;

    for (i = 0; i < 100; i++) {
        item = ini->value(QString("%1_%2").arg(key).arg(i, 3), "").toString();
        if (item != "" && combo->findText(item) == -1) combo->addItem(item);
    }
}
// write history ------------------------------------------------------------
void MainForm::writeList(QSettings *ini, const QString &key, const QComboBox *combo)
{
    int i;

    for (i = 0;i < combo->count(); i++) {
        ini->setValue(QString("%1_%2").arg(key).arg(i, 3), combo->itemText(i));
    }
}
// add history --------------------------------------------------------------
void MainForm::addHistory(QComboBox *combo)
{
    QString hist = combo->currentText();
    if (hist == "") return;
    int i = combo->currentIndex();
    if (i >= 0) combo->removeItem(i);
    combo->insertItem(0, hist);
    for (int i = combo->count() - 1; i >= MAXHIST; i--) combo->removeItem(i);
    combo->setCurrentIndex(0);
}
// execute command ----------------------------------------------------------
int MainForm::execCommand(const QString &cmd, const QStringList &opt, int show)
{
    Q_UNUSED(show);
    return QProcess::startDetached(cmd, opt);  /* FIXME: show option not yet supported */
}
// view file ----------------------------------------------------------------
void MainForm::viewFile(const QString &file)
{
    QString f;
    char tmpfile[1024];
    int cstat;

    if (file == "") return;
    cstat = rtk_uncompress(qPrintable(file), tmpfile);
    f = !cstat ? file : tmpfile;

    textViewer->setWindowTitle(file);
    textViewer->show();
    if (!textViewer->read(f)) textViewer->close();
    if (cstat == 1) remove(tmpfile);
}
// show message in message area ---------------------------------------------
void MainForm::showMessage(const QString &msg)
{
    lblMessage->setText(msg);
}
// get time from time-1 -----------------------------------------------------
gtime_t MainForm::getTimeStart()
{
    QDateTime time(dtDateTimeStart->dateTime());

    gtime_t t;
    t.time = time.toSecsSinceEpoch();
    t.sec = time.time().msec()/1000;

    return t;
}
// get time from time-2 -----------------------------------------------------
gtime_t MainForm::getTimeStop()
{
    QDateTime time(dtDateTimeStop->dateTime());

    gtime_t t;
    t.time=time.toSecsSinceEpoch();t.sec=time.time().msec()/1000;

    return t;
}
// set time to time-1 -------------------------------------------------------
void MainForm::setTimeStart(gtime_t time)
{
    QDateTime t = QDateTime::fromSecsSinceEpoch(time.time);
    t = t.addMSecs(time.sec*1000);
    dtDateTimeStart->setDateTime(t);
}
// set time to time-2 -------------------------------------------------------
void MainForm::setTimeStop(gtime_t time)
{
    QDateTime t = QDateTime::fromSecsSinceEpoch(time.time);
    t = t.addMSecs(time.sec*1000);
    dtDateTimeStop->setDateTime(t);
}
// update enable/disable of widgets -----------------------------------------
void MainForm::updateEnable()
{
    bool moder = PMODE_DGPS <= positionMode && positionMode <= PMODE_FIXED;

    lblInputFile1->setText(moder ? tr("RINEX OBS: Rover") : tr("RINEX OBS"));
    cBInputFile2->setEnabled(moder);
    btnInputFile2->setEnabled(moder);
    btnInputPlot2->setEnabled(moder);
    btnInputView2->setEnabled(moder);
    btnOutputView1->setEnabled(debugStatus > 0);
    btnOutputView2->setEnabled(debugTrace > 0);
    lblInputFile3->setEnabled(moder);
    dtDateTimeStart->setEnabled(cBTimeStart->isChecked());
    btnTimeStart->setEnabled(cBTimeStart->isChecked());
    dtDateTimeStop->setEnabled(cBTimeEnd->isChecked());
    btnTimeStop->setEnabled(cBTimeEnd->isChecked());
    cBTimeInterval->setEnabled(cBTimeIntervalF->isChecked());
    lblTimeInterval->setEnabled(cBTimeIntervalF ->isChecked());
    cBTimeUnitF->setEnabled(cBTimeStart->isChecked() && cBTimeEnd->isChecked());
    lETimeUnit->setEnabled(cBTimeUnitF->isEnabled() && cBTimeUnitF->isChecked());
    lblTimeUnit ->setEnabled(cBTimeUnitF->isEnabled() && cBTimeUnitF->isChecked());
    lEOutputDirectory->setEnabled(cBOutputDirectoryEnable->isChecked());
    btnOutputDirectory->setEnabled(cBOutputDirectoryEnable->isChecked());
}
// load options from ini file -----------------------------------------------
void MainForm::loadOptions()
{
    QSettings ini(iniFile,QSettings::IniFormat);

    cBTimeStart->setChecked(ini.value("set/timestart", 0).toBool());
    cBTimeEnd->setChecked(ini.value("set/timeend", 0).toBool());
    dtDateTimeStart->setDate(ini.value ("set/timey1", QDate(2000, 01, 01)).toDate());
    dtDateTimeStart->setTime(ini.value ("set/timeh1", QTime(0, 0, 0)).toTime());
    dtDateTimeStop->setDate(ini.value ("set/timey2", QDate(2000, 01, 01)).toDate());
    dtDateTimeStop->setTime(ini.value ("set/timeh2",  QTime(0, 0, 0)).toTime());
    cBTimeIntervalF ->setChecked(ini.value("set/timeintf", 0).toBool());
    cBTimeInterval->setCurrentIndex(ini.value ("set/timeint", 0).toInt());
    cBTimeUnitF->setChecked(ini.value("set/timeunitf", 0).toBool());
    lETimeUnit->setText(ini.value ("set/timeunit", "24").toString());
    cBInputFile1->setCurrentText(ini.value ("set/inputfile1", "").toString());
    cBInputFile2->setCurrentText(ini.value ("set/inputfile2", "").toString());
    cBInputFile3->setCurrentText(ini.value ("set/inputfile3", "").toString());
    cBInputFile4->setCurrentText(ini.value ("set/inputfile4", "").toString());
    cBInputFile5->setCurrentText(ini.value ("set/inputfile5", "").toString());
    cBInputFile6->setCurrentText(ini.value ("set/inputfile6", "").toString());
    cBOutputDirectoryEnable->setChecked(ini.value("set/outputdirena", 0).toBool());
    lEOutputDirectory->setText(ini.value ("set/outputdir", "").toString());
    cBOutputFile->setCurrentText(ini.value ("set/outputfile", "").toString());

    readList(cBInputFile1,&ini,"hist/inputfile1");
    readList(cBInputFile2,&ini,"hist/inputfile2");
    readList(cBInputFile3,&ini,"hist/inputfile3");
    readList(cBInputFile4,&ini,"hist/inputfile4");
    readList(cBInputFile5,&ini,"hist/inputfile5");
    readList(cBInputFile6,&ini,"hist/inputfile6");
    readList(cBOutputFile,&ini,"hist/outputfile");

    positionMode = ini.value("opt/posmode", 2).toInt();
    frequencies = ini.value("opt/freq", 1).toInt();
    solution = ini.value("opt/solution", 0).toInt();
    elevationMask = ini.value  ("opt/elmask", 15.0).toDouble();
    snrMask.ena[0] = ini.value("opt/snrmask_ena1", 0).toInt();
    snrMask.ena[1] = ini.value("opt/snrmask_ena2", 0).toInt();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 9; j++) {
            snrMask.mask[i][j] = ini.value(QString("opt/snrmask_%1_%2").arg(i + 1).arg(j + 1), 0.0).toDouble();
    }
    ionosphereOption = ini.value("opt/ionoopt", IONOOPT_BRDC).toInt();
    troposphereOption = ini.value("opt/tropopt", TROPOPT_SAAS).toInt();
    receiverBiasEstimation = ini.value("opt/rcvbiasest",  0).toInt();
    dynamicModel = ini.value("opt/dynamicmodel", 1).toInt();
    tideCorrection = ini.value("opt/tidecorr", 0).toInt();
    satelliteEphemeris = ini.value("opt/satephem", 0).toInt();
    excludedSatellites = ini.value("opt/exsats",  "").toString();
    navigationSystems = ini.value("opt/navsys", SYS_GPS | SYS_GLO | SYS_GAL).toInt();
    positionOption[0] = ini.value("opt/posopt1", 0).toInt();
    positionOption[1] = ini.value("opt/posopt2", 0).toInt();
    positionOption[2] = ini.value("opt/posopt3", 0).toInt();
    positionOption[3] = ini.value("opt/posopt4", 0).toInt();
    positionOption[4] = ini.value("opt/posopt5", 0).toInt();
    positionOption[5] = ini.value("opt/posopt6", 0).toInt();
    mapFunction = ini.value("opt/mapfunc", 0).toInt();

    ambiguityResolutionGPS = ini.value("opt/ambres", 3).toInt();
    ambiguityResolutionGLO = ini.value("opt/gloambres", 3).toInt();
    ambiguityResolutionBDS = ini.value("opt/bdsambres", 0).toInt();
    validThresAR  = ini.value("opt/validthresar", 3.0).toDouble();
    maxPositionVarAR  = ini.value("opt/maxposvarar", 0.1).toDouble();
    glonassHwBias  = ini.value("opt/glohwbias", 0).toDouble();
    thresAR3 = ini.value("opt/thresar3", 1e-9).toDouble();
    thresAR4 = ini.value("opt/thresar4", 1e-5).toDouble();
    validThresARMin = ini.value("opt/validthresarmin", 3.0).toDouble();
    validThresARMax = ini.value("opt/validthresarmax", 3.0).toDouble();
    LockCntFixAmbiguity = ini.value("opt/lockcntfixamb",  0).toInt();
    fixCntHoldAmbiguity = ini.value("opt/fixcntholdamb", 20).toInt();
    elevationMaskAR = ini.value("opt/elmaskar", 15.0).toDouble();
    elevationMaskHold = ini.value("opt/elmaskhold", 15.0).toDouble();
    outputCntResetAmbiguity = ini.value("opt/outcntresetbias", 2).toInt();
    slipThreshold = ini.value("opt/slipthres",   0.05).toDouble();
    dopplerThreshold = ini.value("opt/dopthres",   0.0).toDouble();
    maxAgeDiff  = ini.value("opt/maxagediff", 30.0).toDouble();
    rejectPhase = ini.value("opt/rejectthres", 5.0).toDouble();
    varHoldAmb = ini.value("opt/varholdmb", 0.1).toDouble();
    gainHoldAmb = ini.value("opt/gainholdamb", 0.01).toDouble();
    rejectCode = ini.value("opt/rejectcode",  30.0).toDouble();
    ARIter = ini.value("opt/ariter", 1).toInt();
    numIter = ini.value("opt/numiter", 1).toInt();
    minFixSats = ini.value("opt/minfixsats", 4).toInt();
    minHoldSats = ini.value("opt/minholdsats", 5).toInt();
    minDropSats = ini.value("opt/mindropsats", 10).toInt();
    ARFilter = ini.value("opt/arfilter", 1).toInt();
    codeSmooth = ini.value("opt/codesmooth",     0).toInt();
    baseLine[0] = ini.value("opt/baselinelen",  0.0).toDouble();
    baseLine[1] = ini.value("opt/baselinesig",  0.0).toDouble();
    baseLineConstrain = ini.value("opt/baselineconst",  0).toInt();

    solutionFormat = ini.value("opt/solformat", 0).toInt();
    timeFormat = ini.value("opt/timeformat", 1).toInt();
    timeDecimal = ini.value("opt/timedecimal", 3).toInt();
    latLonFormat = ini.value("opt/latlonformat", 0).toInt();
    fieldSeperator = ini.value("opt/fieldsep", "").toString();
    outputHeader = ini.value("opt/outputhead", 1).toInt();
    outputOptions = ini.value("opt/outputopt", 1).toInt();
    outputVelocity = ini.value("opt/outputvel", 0).toInt();
    outputSingle = ini.value("opt/outputsingle", 0).toInt();
    maxSolutionStd = ini.value("opt/maxsolstd", 0.0).toInt();
    outputDatum = ini.value("opt/outputdatum", 0).toInt();
    outputHeight = ini.value("opt/outputheight", 0).toInt();
    outputGeoid = ini.value("opt/outputgeoid", 0).toInt();
    solutionStatic = ini.value("opt/solstatic", 0).toInt();
    debugTrace = ini.value("opt/debugtrace", 0).toInt();
    debugStatus = ini.value("opt/debugstatus", 0).toInt();

    measurementErrorR1 = ini.value("opt/measeratio1", 300.0).toDouble();
    measurementErrorR2 = ini.value("opt/measeratio2", 300.0).toDouble();
    measurementErrorR5 = ini.value("opt/measeratio5", 300.0).toDouble();
    measurementError2 = ini.value("opt/measerr2", 0.003).toDouble();
    measurementError3 = ini.value("opt/measerr3", 0.003).toDouble();
    measurementError4 = ini.value("opt/measerr4", 0.000).toDouble();
    measurementError5 = ini.value("opt/measerr5", 1.000).toDouble();
    measurementError6 = ini.value("opt/measerr6", 52.000).toDouble();
    measurementError7 = ini.value("opt/measerr7", 0.000).toDouble();
    measurementError8 = ini.value("opt/measerr8",  0.000).toDouble();
    satelliteClockStability = ini.value("opt/satclkstab", 5E-12).toDouble();
    processNoise1 = ini.value("opt/prnoise1", 1E-4).toDouble();
    processNoise2 = ini.value("opt/prnoise2", 1E-3).toDouble();
    processNoise3 = ini.value("opt/prnoise3", 1E-4).toDouble();
    processNoise4 = ini.value("opt/prnoise4", 3E+1).toDouble();
    processNoise5 = ini.value("opt/prnoise5", 1E+1).toDouble();

    roverPositionType = ini.value("opt/rovpostype", 0).toInt();
    referencePositionType = ini.value("opt/refpostype", 5).toInt();
    roverPosition[0] = ini.value("opt/rovpos1", 0.0).toDouble();
    roverPosition[1] = ini.value("opt/rovpos2", 0.0).toDouble();
    roverPosition[2] = ini.value("opt/rovpos3", 0.0).toDouble();
    referencePosition[0] = ini.value("opt/refpos1", 0.0).toDouble();
    referencePosition[1] = ini.value("opt/refpos2", 0.0).toDouble();
    referencePosition[2] = ini.value("opt/refpos3", 0.0).toDouble();
    roverAntennaPcv = ini.value("opt/rovantpcv", 0).toInt();
    referenceAntennaPcv = ini.value("opt/refantpcv", 0).toInt();
    roverAntenna = ini.value("opt/rovant", "").toString();
    referenceAntenna = ini.value("opt/refant", "").toString();
    roverAntennaE = ini.value("opt/rovante", 0.0).toDouble();
    roverAntennaN = ini.value("opt/rovantn", 0.0).toDouble();
    roverAntennaU = ini.value("opt/rovantu", 0.0).toDouble();
    referenceAntennaE = ini.value("opt/refante", 0.0).toDouble();
    referenceAntennaN = ini.value("opt/refantn", 0.0).toDouble();
    referenceAntennaU = ini.value("opt/refantu", 0.0).toDouble();

    rnxOptions1 = ini.value ("opt/rnxopts1", "").toString();
    rnxOptions2 = ini.value ("opt/rnxopts2", "").toString();
    pppOptions = ini.value ("opt/pppopts", "").toString();

    antennaPcvFile = ini.value("opt/antpcvfile", "").toString();
    intpolateReferenceObs = ini.value("opt/intprefobs", 0).toInt();
    sbasSat = ini.value("opt/sbassat", 0).toInt();
    netRSCorr = ini.value("opt/netrscorr", 0).toInt();
    satelliteClockCorrection = ini.value("opt/satclkcorr", 0).toInt();
    sbasCorrection = ini.value("opt/sbascorr", 0).toInt();
    sbasCorrection1 = ini.value("opt/sbascorr1", 0).toInt();
    sbasCorrection2 = ini.value("opt/sbascorr2", 0).toInt();
    sbasCorrection3 = ini.value("opt/sbascorr3", 0).toInt();
    sbasCorrection4 = ini.value("opt/sbascorr4", 0).toInt();
    sbasCorrectionFile = ini.value("opt/sbascorrfile", "").toString();
    PrecEphFile = ini.value("opt/precephfile", "").toString();
    satellitePcvFile = ini.value("opt/satpcvfile",  "").toString();
    stationPositionFile = ini.value("opt/staposfile", "").toString();
    geoidDataFile = ini.value("opt/geoiddatafile", "").toString();
    ionosphereFile = ini.value("opt/ionofile", "").toString();
    eopFile = ini.value("opt/eopfile", "").toString();
    dcbFile = ini.value("opt/dcbfile", "").toString();
    blqFile = ini.value("opt/blqfile", "").toString();
    googleEarthFile = ini.value("opt/googleearthfile", GOOGLE_EARTH).toString();

    roverList = "";
    for (int i = 0; i < 10; i++) {
        roverList += ini.value(QString("opt/rovlist%1").arg(i + 1), "").toString();
    }
    baseList = "";
    for (int i = 0; i < 10; i++) {
        baseList += ini.value(QString("opt/baselist%1").arg(i + 1), "").toString();
    }
    roverList.replace("@@","\n");
    baseList.replace("@@","\n");

    convDialog->cBTimeSpan->setChecked(ini.value("conv/timespan", 0).toInt());
    convDialog->cBTimeInterval->setChecked(ini.value("conv/timeintf", 0).toInt());
    convDialog->dateTimeStart->setDate(ini.value("conv/timey1", QDate(2000, 1, 1)).toDate());
    convDialog->dateTimeStart->setTime(ini.value("conv/timeh1", QTime(0, 0, 0)).toTime());
    convDialog->dateTimeStop->setDate(ini.value("conv/timey2", QDate(2000, 1, 1)).toDate());
    convDialog->dateTimeStop->setTime(ini.value("conv/timeh2", QTime(0, 0, 0)).toTime());
    convDialog->sBTimeInterval->setValue(ini.value ("conv/timeint", 0.0).toDouble());
    convDialog->cBTrackColor->setCurrentIndex(ini.value("conv/trackcolor", 5).toInt());
    convDialog->cBPointColor->setCurrentIndex(ini.value("conv/pointcolor", 5).toInt());
    convDialog->cBOutputAltitude->setCurrentIndex(ini.value("conv/outputalt", 0).toInt());
    convDialog->cBOutputTime->setCurrentIndex(ini.value("conv/outputtime",0).toInt());
    convDialog->cBAddOffset->setChecked(ini.value("conv/addoffset", 0).toInt());
    convDialog->sBOffset1->setValue(ini.value("conv/offset1", 0).toDouble());
    convDialog->sBOffset2->setValue(ini.value("conv/offset2", 0).toDouble());
    convDialog->sBOffset3->setValue(ini.value("conv/offset3", 0).toDouble());
    convDialog->cBCompress->setChecked(ini.value("conv/compress", 0).toInt());
    convDialog->rBFormatKML->setChecked(ini.value("conv/format", 0).toInt());

    textViewer->colorText = ini.value("viewer/color1", QColor(Qt::black)).value<QColor>();
    textViewer->colorBackground = ini.value("viewer/color2", QColor(Qt::white)).value<QColor>();
    textViewer->font.setFamily(ini.value ("viewer/fontname", "Courier New").toString());
    textViewer->font.setPointSize(ini.value("viewer/fontsize", 9).toInt());
}
// save options to ini file -------------------------------------------------
void MainForm::saveOptions()
{
    QSettings ini(iniFile,QSettings::IniFormat);

    ini.setValue("set/timestart", cBTimeStart->isChecked() ? 1 : 0);
    ini.setValue("set/timeend", cBTimeEnd->isChecked()? 1 : 0);
    ini.setValue("set/timey1", dtDateTimeStart->date());
    ini.setValue("set/timeh1", dtDateTimeStart->time());
    ini.setValue("set/timey2", dtDateTimeStop->date());
    ini.setValue("set/timeh2", dtDateTimeStop->time());
    ini.setValue("set/timeintf", cBTimeIntervalF->isChecked() ? 1 : 0);
    ini.setValue("set/timeint", cBTimeInterval->currentText());
    ini.setValue("set/timeunitf", cBTimeUnitF->isChecked() ? 1 : 0);
    ini.setValue("set/timeunit", lETimeUnit->text());
    ini.setValue("set/inputfile1", cBInputFile1->currentText());
    ini.setValue("set/inputfile2", cBInputFile2->currentText());
    ini.setValue("set/inputfile3", cBInputFile3->currentText());
    ini.setValue("set/inputfile4", cBInputFile4->currentText());
    ini.setValue("set/inputfile5", cBInputFile5->currentText());
    ini.setValue("set/inputfile6", cBInputFile6->currentText());
    ini.setValue("set/outputdirena", cBOutputDirectoryEnable->isChecked());
    ini.setValue("set/outputdir", lEOutputDirectory->text());
    ini.setValue("set/outputfile", cBOutputFile->currentText());

    writeList(&ini,"hist/inputfile1", cBInputFile1);
    writeList(&ini,"hist/inputfile2", cBInputFile2);
    writeList(&ini,"hist/inputfile3", cBInputFile3);
    writeList(&ini,"hist/inputfile4", cBInputFile4);
    writeList(&ini,"hist/inputfile5", cBInputFile5);
    writeList(&ini,"hist/inputfile6", cBInputFile6);
    writeList(&ini,"hist/outputfile", cBOutputFile);

    ini.setValue("opt/posmode", positionMode);
    ini.setValue("opt/freq", frequencies);
    ini.setValue("opt/solution", solution);
    ini.setValue ("opt/elmask", elevationMask);
    ini.setValue("opt/snrmask_ena1", snrMask.ena[0]);
    ini.setValue("opt/snrmask_ena2", snrMask.ena[1]);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 9; j++) {
            ini.setValue(QString("opt/snrmask_%1_%2").arg(i + 1).arg(j + 1), snrMask.mask[i][j]);
    }
    ini.setValue("opt/ionoopt", ionosphereOption);
    ini.setValue("opt/tropopt", troposphereOption);
    ini.setValue("opt/rcvbiasest", receiverBiasEstimation);
    ini.setValue("opt/dynamicmodel", dynamicModel);
    ini.setValue("opt/tidecorr", tideCorrection);
    ini.setValue("opt/satephem", satelliteEphemeris);
    ini.setValue("opt/exsats", excludedSatellites);
    ini.setValue("opt/navsys", navigationSystems);
    ini.setValue("opt/posopt1", positionOption[0]);
    ini.setValue("opt/posopt2", positionOption[1]);
    ini.setValue("opt/posopt3", positionOption[2]);
    ini.setValue("opt/posopt4", positionOption[3]);
    ini.setValue("opt/posopt5", positionOption[4]);
    ini.setValue("opt/posopt6", positionOption[5]);
    ini.setValue("opt/mapfunc", mapFunction);

    ini.setValue("opt/ambres", ambiguityResolutionGPS      );
    ini.setValue("opt/gloambres", ambiguityResolutionGLO);
    ini.setValue("opt/bdsambres", ambiguityResolutionBDS);
    ini.setValue("opt/validthresar", validThresAR);
    ini.setValue("opt/maxposvarar", validThresAR);
    ini.setValue("opt/glohwbias", glonassHwBias);
    ini.setValue("opt/thresar3", thresAR3);
    ini.setValue("opt/thresar4", thresAR4);
    ini.setValue("opt/validthresarmin", validThresARMin);
    ini.setValue("opt/validthresarmax", validThresARMax);
    ini.setValue("opt/lockcntfixamb", LockCntFixAmbiguity);
    ini.setValue("opt/fixcntholdamb", fixCntHoldAmbiguity);
    ini.setValue("opt/elmaskar", elevationMaskAR);
    ini.setValue("opt/elmaskhold", elevationMaskHold);
    ini.setValue("opt/outcntresetbias", outputCntResetAmbiguity);
    ini.setValue("opt/slipthres", slipThreshold);
    ini.setValue("opt/dopthres", dopplerThreshold);
    ini.setValue("opt/maxagediff", maxAgeDiff);
    ini.setValue("opt/rejectcode", rejectCode);
    ini.setValue("opt/rejectthres", rejectPhase);
    ini.setValue("opt/varholdamb", varHoldAmb);
    ini.setValue("opt/gainholdamb", gainHoldAmb);
    ini.setValue("opt/ariter", ARIter);
    ini.setValue("opt/numiter", numIter);
    ini.setValue("opt/minfixsats", minFixSats);
    ini.setValue("opt/minholdsats", minHoldSats);
    ini.setValue("opt/mindropsats", minDropSats);
    ini.setValue("opt/arfilter", ARFilter);
    ini.setValue("opt/codesmooth", codeSmooth);
    ini.setValue("opt/baselinelen", baseLine[0]);
    ini.setValue("opt/baselinesig", baseLine[1]);
    ini.setValue("opt/baselineconst",baseLineConstrain);

    ini.setValue("opt/solformat", solutionFormat);
    ini.setValue("opt/timeformat", timeFormat);
    ini.setValue("opt/timedecimal", timeDecimal);
    ini.setValue("opt/latlonformat", latLonFormat);
    ini.setValue("opt/fieldsep", fieldSeperator);
    ini.setValue("opt/outputhead", outputHeader);
    ini.setValue("opt/outputopt", outputOptions);
    ini.setValue("opt/outputvel", outputVelocity);
    ini.setValue("opt/outputsingle", outputSingle);
    ini.setValue("opt/maxsolstd", maxSolutionStd);
    ini.setValue("opt/outputdatum", outputDatum);
    ini.setValue("opt/outputheight", outputHeight);
    ini.setValue("opt/outputgeoid", outputGeoid);
    ini.setValue("opt/solstatic", solutionStatic);
    ini.setValue("opt/debugtrace", debugTrace);
    ini.setValue("opt/debugstatus", debugStatus);

    ini.setValue("opt/measeratio1", measurementErrorR1);
    ini.setValue("opt/measeratio2", measurementErrorR2);
    ini.setValue("opt/measeratio5", measurementErrorR5);
    ini.setValue("opt/measerr2", measurementError2);
    ini.setValue("opt/measerr3", measurementError3);
    ini.setValue("opt/measerr4", measurementError4);
    ini.setValue("opt/measerr5", measurementError5);
    ini.setValue("opt/measerr6", measurementError6);
    ini.setValue("opt/measerr7", measurementError7);
    ini.setValue("opt/measerr8", measurementError8);
    ini.setValue("opt/satclkstab", satelliteClockStability);
    ini.setValue("opt/prnoise1", processNoise1);
    ini.setValue("opt/prnoise2", processNoise2);
    ini.setValue("opt/prnoise3", processNoise3);
    ini.setValue("opt/prnoise4", processNoise4);
    ini.setValue("opt/prnoise5",  processNoise5);

    ini.setValue("opt/rovpostype", roverPositionType);
    ini.setValue("opt/refpostype", referencePositionType);
    ini.setValue("opt/rovpos1", roverPosition[0]);
    ini.setValue("opt/rovpos2", roverPosition[1]);
    ini.setValue("opt/rovpos3", roverPosition[2]);
    ini.setValue("opt/refpos1", referencePosition[0]);
    ini.setValue("opt/refpos2", referencePosition[1]);
    ini.setValue("opt/refpos3", referencePosition[2]);
    ini.setValue("opt/rovantpcv", roverAntennaPcv);
    ini.setValue("opt/refantpcv", referenceAntennaPcv);
    ini.setValue("opt/rovant", roverAntenna);
    ini.setValue("opt/refant", referenceAntenna);
    ini.setValue("opt/rovante", roverAntennaE);
    ini.setValue("opt/rovantn", roverAntennaN);
    ini.setValue("opt/rovantu", roverAntennaU);
    ini.setValue("opt/refante", referenceAntennaE);
    ini.setValue("opt/refantn", referenceAntennaN);
    ini.setValue("opt/refantu", referenceAntennaU);

    ini.setValue("opt/rnxopts1", rnxOptions1);
    ini.setValue("opt/rnxopts2", rnxOptions2);
    ini.setValue("opt/pppopts", pppOptions);

    ini.setValue("opt/antpcvfile", antennaPcvFile);
    ini.setValue("opt/intprefobs", intpolateReferenceObs);
    ini.setValue("opt/sbassat", sbasSat);
    ini.setValue("opt/netrscorr", netRSCorr);
    ini.setValue("opt/satclkcorr", satelliteClockCorrection);
    ini.setValue("opt/sbascorr", sbasCorrection);
    ini.setValue("opt/sbascorr1", sbasCorrection1);
    ini.setValue("opt/sbascorr2", sbasCorrection2);
    ini.setValue("opt/sbascorr3", sbasCorrection3);
    ini.setValue("opt/sbascorr4", sbasCorrection4 );
    ini.setValue("opt/sbascorrfile", sbasCorrectionFile);
    ini.setValue("opt/precephfile", PrecEphFile);
    ini.setValue("opt/satpcvfile", satellitePcvFile);
    ini.setValue("opt/staposfile", stationPositionFile);
    ini.setValue("opt/geoiddatafile", geoidDataFile);
    ini.setValue("opt/ionofile", ionosphereFile);
    ini.setValue("opt/eopfile", eopFile);
    ini.setValue("opt/dcbfile", dcbFile);
    ini.setValue("opt/blqfile", blqFile);
    ini.setValue("opt/googleearthfile", googleEarthFile);

    roverList.replace("\n", "@@");
    for (int i = 0; i < 10; i++) {
        ini.setValue(QString("opt/rovlist%1").arg(i + 1), roverList.mid(i*2000, 2000));
    }

    baseList.replace("\n", "@@");
    for (int i = 0; i < 10; i++) {
        ini.setValue(QString("opt/baselist%1").arg(i + 1), baseList.mid(i*2000, 2000));
    }

    ini.setValue ("conv/timespan", convDialog->cBTimeSpan->isChecked());
    ini.setValue ("conv/timey1", convDialog->dateTimeStart->date());
    ini.setValue ("conv/timeh1", convDialog->dateTimeStart->time());
    ini.setValue ("conv/timey2", convDialog->dateTimeStop->date());
    ini.setValue ("conv/timeh2", convDialog->dateTimeStop->time());
    ini.setValue ("conv/timeintf", convDialog->cBTimeInterval->isChecked());
    ini.setValue ("conv/timeint", convDialog->sBTimeInterval->text());
    ini.setValue ("conv/trackcolor", convDialog->cBTrackColor->currentIndex());
    ini.setValue ("conv/pointcolor", convDialog->cBPointColor->currentIndex());
    ini.setValue ("conv/outputalt", convDialog->cBOutputAltitude->currentIndex());
    ini.setValue ("conv/outputtime", convDialog->cBOutputTime->currentIndex());
    ini.setValue ("conv/addoffset", convDialog->cBAddOffset ->isChecked());
    ini.setValue ("conv/offset1", convDialog->sBOffset1->text());
    ini.setValue ("conv/offset2", convDialog->sBOffset2->text());
    ini.setValue ("conv/offset3", convDialog->sBOffset3->text());
    ini.setValue ("conv/compress", convDialog->cBCompress->isChecked());
    ini.setValue ("conv/format", convDialog->rBFormatKML->isChecked());

    ini.setValue ("viewer/color1", textViewer->colorText);
    ini.setValue ("viewer/color2", textViewer->colorBackground);
    ini.setValue ("viewer/fontname", textViewer->font.family());
    ini.setValue ("viewer/fontsize", textViewer->font.pointSize());
}
//---------------------------------------------------------------------------

