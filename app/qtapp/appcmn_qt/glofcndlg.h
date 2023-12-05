//---------------------------------------------------------------------------

#ifndef glofcndlgH
#define glofcndlgH
//---------------------------------------------------------------------------

#include <QDialog>
#include <QLineEdit>
#include "ui_glofcndlg.h"

//---------------------------------------------------------------------------
class GloFcnDialog : public QDialog, private Ui::GloFcnDialog
{
    Q_OBJECT

public slots:
    void btnOkClicked();
    void btnReadClicked();
    void btnClearClicked();
    void updateEnable(void);
private:
    QSpinBox * getFcn(int prn);

protected:
    void showEvent(QShowEvent*);

public:
    GloFcnDialog(QWidget *parent);

    int enableGloFcn, gloFcn[27];
};

//---------------------------------------------------------------------------
#endif
