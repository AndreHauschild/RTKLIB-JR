//---------------------------------------------------------------------------
#ifndef aboutdlgH
#define aboutdlgH
//---------------------------------------------------------------------------
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QPixmap>

namespace Ui {
    class AboutDlg;
}

//---------------------------------------------------------------------------
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget*, QPixmap icon, QString labelText);

private:
    Ui::AboutDlg *ui;
};
#endif
