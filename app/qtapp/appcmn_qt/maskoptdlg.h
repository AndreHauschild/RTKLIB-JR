//---------------------------------------------------------------------------
#ifndef maskoptdlgH
#define maskoptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

#include "rtklib.h"

#include "ui_maskoptdlg.h"

//---------------------------------------------------------------------------
class MaskOptDialog : public QDialog, private Ui::MaskOptDialog
{
    Q_OBJECT
protected:
    void  showEvent(QShowEvent*);

public slots:
    void  btnOkClicked();
    void  updateEnable(void);

public:
    snrmask_t mask;
    explicit MaskOptDialog(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
