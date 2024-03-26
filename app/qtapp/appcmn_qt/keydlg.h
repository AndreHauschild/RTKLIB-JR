//---------------------------------------------------------------------------

#ifndef keydlgH
#define keydlgH

#include <QDialog>

namespace Ui {
class KeyDialog;
}

//---------------------------------------------------------------------------
class KeyDialog : public QDialog
{
    Q_OBJECT

private:
    Ui::KeyDialog *ui;

public:
    explicit KeyDialog(QWidget* parent, int flags = 0);

    // 0 (default): hide station id key;
    // 1: show hour keys, hide station sequence keys
    // 2: show base station key, hide station sequence keys;
    // 3: show station sequence keys (no hour keys)
    void setFlag(int flag);
};
//---------------------------------------------------------------------------
#endif
