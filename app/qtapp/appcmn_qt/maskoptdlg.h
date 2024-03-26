//---------------------------------------------------------------------------
#ifndef maskoptdlgH
#define maskoptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

#include "rtklib.h"

namespace Ui {
class MaskOptDialog;
}

//---------------------------------------------------------------------------
class MaskOptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MaskOptDialog(QWidget* parent = nullptr);

    snrmask_t getSnrMask();
    void setSnrMask(snrmask_t);

protected slots:
    void  updateEnable();

private:
    Ui::MaskOptDialog *ui;
};
//---------------------------------------------------------------------------
#endif
