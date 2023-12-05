//---------------------------------------------------------------------------
#ifndef conndlgH
#define conndlgH
//---------------------------------------------------------------------------

#define MAXHIST		10

#include "ui_conndlg.h"

#include <QDialog>

class QShowEvent;

//---------------------------------------------------------------------------
class ConnectDialog : public QDialog, private Ui::ConnectDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent*);

public slots:
    void btnOkClick();
    void btnOption1Clicked();
    void btnOption2Clicked();
    void btnCommand1Clicked();
    void btnCommand2Clicked();
    void updateEnable(void);

private:
    void serialOption1(int opt);
    void serialOption2(int opt);
    void tcpOption1(int opt);
    void tcpOption2(int opt);
    void fileOption1(int opt);
    void fileOption2(int opt);

public:
    int stream1, stream2, format1, format2, commandEnable1[2], commandEnable2[2];
    int timeFormat, degFormat, timeoutTime, reconnectTime;
    QString path, paths1[4], paths2[4];
    QString TcpHistory[MAXHIST];
    QString commands1[2], commands2[2], fieldSeparator;

    explicit ConnectDialog(QWidget *parent=NULL);
};
//---------------------------------------------------------------------------
#endif
