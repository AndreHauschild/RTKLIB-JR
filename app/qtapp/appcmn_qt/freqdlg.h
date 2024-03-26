//---------------------------------------------------------------------------

#ifndef freqdlgH
#define freqdlgH

#include <QDialog>

//---------------------------------------------------------------------------
namespace Ui {
class FreqDialog;
}

//---------------------------------------------------------------------------
class FreqDialog : public QDialog
{
private:
    Ui::FreqDialog *ui;

public:
    FreqDialog(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
