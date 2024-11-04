//---------------------------------------------------------------------------
#ifndef mntpoptdlgH
#define mntpoptdlgH
//---------------------------------------------------------------------------

#include <QDialog>

namespace Ui {
class MntpOptDialog;
}

//---------------------------------------------------------------------------
class MntpOptDialog : public QDialog
{
    Q_OBJECT

public:
    MntpOptDialog(QWidget* parent = nullptr, int option = 0);
    void setOption(int option);

    QString getMountPoint();
    void setMountPoint(QString mountPoint);
    QString getMountPointString();
    void setMountPointString(QString mountPointString);


private:
    Ui::MntpOptDialog *ui;
};

//---------------------------------------------------------------------------
#endif
