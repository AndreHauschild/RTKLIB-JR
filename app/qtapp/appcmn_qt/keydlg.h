//---------------------------------------------------------------------------

#ifndef keydlgH
#define keydlgH

#include <QDialog>

#include "ui_keydlg.h"

//---------------------------------------------------------------------------
class KeyDialog : public QDialog, private Ui::KeyDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent*);

public:
    int flag;  // 0 (default): hide station id key; 1: show hour keys, hide station sequence keys 2: show base station key, hide station sequence keys; 3: show station sequence keys (no hour keys)

    explicit KeyDialog(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
