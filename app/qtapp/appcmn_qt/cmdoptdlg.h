//---------------------------------------------------------------------------
#ifndef cmdoptdlgH
#define cmdoptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

#include "ui_cmdoptdlg.h"

//---------------------------------------------------------------------------
class CmdOptDialog : public QDialog, private Ui_CmdOptDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent *);

public slots:
    void btnOkClicked();
    void btnLoadClicked();
    void btnSaveClicked();
    void updateEnable();

public:
    QString commands[3];
    bool commandsEnabled[3];
    explicit CmdOptDialog(QWidget* parent);
};
#endif
