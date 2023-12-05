//---------------------------------------------------------------------------
#ifndef codeoptH
#define codeoptH
//---------------------------------------------------------------------------
#include <QDialog>

#include "ui_codeopt.h"

class QShowEvent;
class ConvOptDialog;

//---------------------------------------------------------------------------
class CodeOptDialog : public QDialog, private Ui::CodeOptDialog
{
    Q_OBJECT

public slots:
    void btnOkClicked();
    void btnSetAllClicked();

protected:
    void showEvent(QShowEvent*);

private:
    void updateEnable(void);

public:
    int navSystem, frequencyType;
    QString codeMask[7];

    CodeOptDialog(QWidget *parent);
};
//----------------------------------------------------------------------
#endif
