//---------------------------------------------------------------------------
#ifndef serioptdlgH
#define serioptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class SerialOptDialog;
}

class CmdOptDialog;
//---------------------------------------------------------------------------
class SerialOptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SerialOptDialog(QWidget*, int options = 0);

    void setOptions(int options);
    QString getPath();
    void setPath(const QString &path);

protected:
    void updatePortList();
    void showEvent(QShowEvent *event);

private:
    Ui::SerialOptDialog *ui;

protected slots:
    void updateEnable();

};
//---------------------------------------------------------------------------
#endif
