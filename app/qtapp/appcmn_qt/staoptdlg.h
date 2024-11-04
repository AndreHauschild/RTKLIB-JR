//---------------------------------------------------------------------------
#ifndef staoptdlgH
#define staoptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class StaListDialog;
}

//---------------------------------------------------------------------------
class StaListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StaListDialog(QWidget* parent);

    void setStationList(const QStringList&);
    QStringList getStationList();

private:
    Ui::StaListDialog *ui;

public slots:
    void  loadFile();
    void  saveFile();
};
//---------------------------------------------------------------------------
#endif
