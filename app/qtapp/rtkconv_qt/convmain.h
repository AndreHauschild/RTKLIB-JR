//---------------------------------------------------------------------------
#ifndef convmainH
#define convmainH
//---------------------------------------------------------------------------
#include <QMainWindow>
#include <QThread>

#include "rtklib.h"

namespace Ui {
class MainWindow;
}

class QShowEvent;
class QCloseEvent;
class QSettings;
class QComboBox;
class ConvOptDialog;
class TimeDialog;
class KeyDialog;
class AboutDialog;
class StartDialog;
class TextViewer;

// Conversion Thread Class ------------------------------------------------------------------
// to allow conversion in background without blocking the GUI
class ConversionThread : public QThread
{
    Q_OBJECT
public:
    char ifile[1024], *ofile[9];
    rnxopt_t rnxopt;
    int format;

    explicit ConversionThread(QObject *parent) : QThread(parent) {
        for (int i = 0; i < 9; i++)
        {
            ofile[i] = new char[1024];
            ofile[i][0] = '\0';
        };
        memset(&rnxopt, 0, sizeof(rnxopt_t));
        format = 0;
        ifile[0] = '\0';
    }

    ~ConversionThread() {
        for (int i = 0; i < 9; i++) delete[] ofile[i];
    }

protected:
    void run() {
        // convert to rinex
        convrnx(format, &rnxopt, ifile, ofile);
    }
};

//---------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    void showMessage(const QString &);

protected:
    void showEvent(QShowEvent*);
    void closeEvent(QCloseEvent*);
    void dragEnterEvent(QDragEnterEvent *);
    void dropEvent(QDropEvent *);

protected slots:
    void callRtkPlot();
    void showOptions();
    void showAboutDialog();
    void showStartTimeDialog();
    void showStopTimeDialog();
    void selectInputFile();
    void selectOutputFile1();
    void selectOutputFile2();
    void selectOutputFile3();
    void selectOutputFile4();
    void selectOutputFile5();
    void selectOutputFile6();
    void selectOutputFile7();
    void selectOutputFile8();
    void selectOutputFile9();
    void viewOutputFile1();
    void viewOutputFile2();
    void viewOutputFile3();
    void viewOutputFile4();
    void viewOutputFile5();
    void viewOutputFile6();
    void viewOutputFile7();
    void viewOutputFile8();
    void viewOutputFile9();
    void abort();
    void outputDirectoryEnableClicked();
	
    void inputFileChanged();
    void outputDirectoryChanged();
    void selectOutputDirectory();
    void showKeyDialog();
    void callRtkPost();
    void viewInputFile();
    void conversionFinished();
    void updateEnable();

    void convertFile();

private:
    QString iniFile, commandPostExe;
    ConversionThread *conversionThread;

    void readHistory(QComboBox* combo, QSettings *ini, const QString &key);
    void writeHistory(QSettings *ini, const QString &key, const QComboBox *combo);
    void addHistory(QComboBox *combo);
	
    int autoFormat(const QString &file);
    void setOutputFiles(const QString &infile);
    void getTime(gtime_t *ts, gtime_t *te, double *tint, double *tunit);
    int  execCommand(const QString &cmd, QStringList &opt);
    QString repPath(const QString &File);
    void loadOptions();
    void saveOptions();
		
    ConvOptDialog *convOptDialog;
    TimeDialog *timeDialog;
    KeyDialog* keyDialog;
    AboutDialog* aboutDialog;
    StartDialog* startDialog;
    TextViewer *viewer;

    Ui::MainWindow *ui;
};
//---------------------------------------------------------------------------
#endif
