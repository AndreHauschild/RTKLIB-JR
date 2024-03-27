//---------------------------------------------------------------------------
#ifndef logstrdlgH
#define logstrdlgH
//---------------------------------------------------------------------------
#include <QDialog>

#include "ui_logstrdlg.h"

class KeyDialog;
class SerialOptDialog;
class TcpOptDialog;

//---------------------------------------------------------------------------
class LogStrDialog : public QDialog, private Ui::LogStrDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent *);

    KeyDialog *keyDialog;
    SerialOptDialog *serialOptDialog;
    TcpOptDialog *tcpOptDialog;

public slots:
    void btnOkClicked();
    void btnStream1Clicked();
    void btnStream2Clicked();
    void btnStream3Clicked();
    void btnFile1Clicked();
    void btnFile2Clicked();
    void btnKeyClicked();
    void btnFile3Clicked();
    void updateEnable();

private:
    QString getFilePath(const QString &path);
    QString setFilePath(const QString &path);

    void serialOptions(int index, int opt);
    void tcpOptions(int index, int opt);

public:
    int streamEnabled[3], stream[3], logTimeTag, logAppend;
    QString paths[3][4], swapInterval;
    QString history[10];

    explicit LogStrDialog(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
