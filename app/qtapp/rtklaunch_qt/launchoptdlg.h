//---------------------------------------------------------------------------

#ifndef launchoptdlgH
#define launchoptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class LaunchOptDialog;
}

class QShowEvent;
//---------------------------------------------------------------------------
class LaunchOptDialog : public QDialog
{
    Q_OBJECT

public:
    LaunchOptDialog(QWidget* parent);

    void setMinimize(int minimized);
    int getMinimize();
    void setOption(int option);
    int getOption();

private:
    Ui::LaunchOptDialog *ui;
};
//---------------------------------------------------------------------------
#endif
