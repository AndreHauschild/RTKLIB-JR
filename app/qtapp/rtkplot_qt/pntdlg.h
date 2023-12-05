//---------------------------------------------------------------------------
#ifndef pntdlgH
#define pntdlgH
//---------------------------------------------------------------------------
#include <QDialog>

#include "ui_pntdlg.h"
//---------------------------------------------------------------------------
class PntDialog : public QDialog, private Ui::PntDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent*);
    bool noUpdate;

public slots:
    void btnDeleteClicked();
    void btnAddClicked();
    void btnUpdateClicked();
    void pntListSetEditText();
    void pntListClicked();
    void pntListDblClicked(QTableWidgetItem *w);

private:
    void updatePoint(void);

public:
    double position[3];
    int fontScale;

    explicit PntDialog(QWidget* parent = NULL);
    void setPoint(void);
};
//---------------------------------------------------------------------------
#endif
