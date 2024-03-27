//---------------------------------------------------------------------------
#ifndef refdlgH
#define refdlgH
//---------------------------------------------------------------------------

#include <QDialog>

#include "ui_refdlg.h"

//---------------------------------------------------------------------------
class RefDialog : public QDialog, public Ui::RefDialog
{
    Q_OBJECT

protected:
    void  showEvent(QShowEvent *);

public slots:
    void  btnOKClicked();
    void  stationListDblClick(int, int);
    void  btnLoadClicked();
    void  findList(void);

private:
    void  loadList(void);
    void  loadSinex(void);
    void  addReference(int n, double *pos, const QString code, const QString name);
    int   selectReference(void);
    void  updateDistances(void);

public:
    QString stationPositionFile, stationId, stationName;
    int options, fontScale;
    double position[3], roverPosition[3];

    explicit RefDialog(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
