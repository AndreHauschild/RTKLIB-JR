//---------------------------------------------------------------------------
#ifndef pntdlgH
#define pntdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class PntDialog;
}

class QTableWidgetItem;
class Plot;

//---------------------------------------------------------------------------
class PntDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PntDialog(Plot * plot, QWidget* parent = NULL);
    void setPoints();

protected:
    void showEvent(QShowEvent*);
    bool noUpdate;

public slots:
    void deletePoint();
    void addPoint();
    void selectPoint();
    void centerPoint(QTableWidgetItem *w);

private:
    void updatePoints();

    Plot *plot;
    Ui::PntDialog * ui;

};
//---------------------------------------------------------------------------
#endif
