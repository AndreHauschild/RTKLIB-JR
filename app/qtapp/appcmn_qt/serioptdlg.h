//---------------------------------------------------------------------------
#ifndef serioptdlgH
#define serioptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

#include "ui_serioptdlg.h"

class CmdOptDialog;
//---------------------------------------------------------------------------
class SerialOptDialog : public QDialog, private Ui::SerialOptDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent *);

    void updatePortList();

    CmdOptDialog *cmdOptDialog;

public slots:
    void  btnOkClicked();
    void updateEnable();

public:
    QString path, commands[2];
    int options, commandsEnabled[2];

    explicit SerialOptDialog(QWidget*);
};
//---------------------------------------------------------------------------
#endif
