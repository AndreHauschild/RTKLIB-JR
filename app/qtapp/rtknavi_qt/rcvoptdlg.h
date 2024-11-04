//---------------------------------------------------------------------------
#ifndef rcvoptdlgH
#define rcvoptdlgH
//---------------------------------------------------------------------------

#include <QDialog>

namespace Ui {
class RcvOptDialog;
}

//---------------------------------------------------------------------------
class RcvOptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RcvOptDialog(QWidget* parent);

    QString getOptions();
    void setOptions(const QString &);

private:
    Ui::RcvOptDialog *ui;
};
//---------------------------------------------------------------------------
#endif
