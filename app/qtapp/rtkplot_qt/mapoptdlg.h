//---------------------------------------------------------------------------
#ifndef mapoptdlgH
#define mapoptdlgH
//---------------------------------------------------------------------------
#include "ui_mapoptdlg.h"

#include <QDialog>

class QKeyEvent;
class QShowEvent;

//---------------------------------------------------------------------------
class MapOptDialog : public QDialog, private Ui::MapAreaDialog
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent*);

public slots:
    void btnCloseClicked();
    void btnUpdateClicked();
    void btnSaveClicked();

    void btnCenterClicked();
    void ScaleEqualClicked();

private:
    void updateMap(void);
    void updatePlot(void);
    void updateEnable(void);
	
public:
    MapOptDialog(QWidget* parent);
    void updateField(void);
};

//---------------------------------------------------------------------------
#endif
