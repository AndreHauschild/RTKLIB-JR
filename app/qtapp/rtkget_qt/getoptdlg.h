//---------------------------------------------------------------------------
#ifndef getoptdlgH
#define getoptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class DownOptDialog;
}
class QSettings;

//---------------------------------------------------------------------------
class DownOptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownOptDialog(QWidget* parent);

    void saveOptions(QSettings &);
    void loadOptions(QSettings &);

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

protected slots:
    void updateUi();
    void saveClose();
    void selectUrlFile();
    void selectLogFilename();

private:
    Ui::DownOptDialog *ui;

};
//---------------------------------------------------------------------------
#endif
