//---------------------------------------------------------------------------
#ifndef rcvoptdlgH
#define rcvoptdlgH
//---------------------------------------------------------------------------

#include <QDialog>
#include <ui_rcvoptdlg.h>
//---------------------------------------------------------------------------
class RcvOptDialog : public QDialog, private Ui::RcvOptDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent *);

public slots:
    void btnOkClicked();

public:
    QString options;

    explicit RcvOptDialog(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
