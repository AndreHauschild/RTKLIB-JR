//---------------------------------------------------------------------------

#ifndef markdlgH
#define markdlgH
//---------------------------------------------------------------------------
#include <QDialog>
#include "ui_markdlg.h"

class KeyDialog;

//---------------------------------------------------------------------------
class QMarkDialog : public QDialog, private Ui::MarkDialog
{
    Q_OBJECT
public slots:
    void btnOkClicked();
    void btnKeyDlgClicked();
    void btnPositionClicked();

protected:
    void showEvent(QShowEvent *);

private:
    void updateEnable(void);
    KeyDialog *keyDialog;

public:
    QString name, comment;
    int positionMode, nMark;
    double fixPosition[3];
	
    explicit QMarkDialog(QWidget *parent);
	
};
//---------------------------------------------------------------------------
#endif
