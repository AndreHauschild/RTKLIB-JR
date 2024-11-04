//---------------------------------------------------------------------------

#ifndef glofcndlgH
#define glofcndlgH
//---------------------------------------------------------------------------

#include <QDialog>

namespace Ui {
class GloFcnDialog;
}

class QSpinBox;

//---------------------------------------------------------------------------
class GloFcnDialog : public QDialog
{
    Q_OBJECT

public:
    GloFcnDialog(QWidget *parent);

    int getGloFcnEnable();
    void setGloFcnEnable(int enable);
    int getGloFcn(int i);
    void setGloFcn(int i, int fcn);

protected slots:
    void readRinex();
    void clearFrequencies();
    void updateEnable();

private:
    QSpinBox * getFcn(int prn);

    Ui::GloFcnDialog *ui;
};

//---------------------------------------------------------------------------
#endif
