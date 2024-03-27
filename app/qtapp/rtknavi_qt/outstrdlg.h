//---------------------------------------------------------------------------
#ifndef outstrdlgH
#define outstrdlgH
//---------------------------------------------------------------------------
#include <QDialog>
#include "ui_outstrdlg.h"

class KeyDialog;
class SerialOptDialog;
class TcpOptDialog;

//---------------------------------------------------------------------------
class OutputStrDialog : public QDialog, private Ui::OutputStrDialog
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
    void btnFile1Clicked();
    void btnFile2Clicked();
    void btnKeyClicked();
    void updateEnable(void);

private:
    QString getFilePath(const QString path);
    QString setFilePath(const QString path);
    void serialOptions(int index, int opt);
    void tcpOptions(int index, int opt);

public:
    int streamEnabled[2], stream[2], format[2], outputTimeTag, outputAppend;
    QString paths[2][4], swapInterval;
    QString history[10];

    explicit OutputStrDialog(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
