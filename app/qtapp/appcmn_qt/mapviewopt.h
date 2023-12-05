//---------------------------------------------------------------------------

#ifndef mapviewoptH
#define mapviewoptH
//---------------------------------------------------------------------------
#include "ui_mapviewopt.h"

#include <QDialog>

//---------------------------------------------------------------------------
class MapViewOptDialog : public QDialog, private Ui::MapViewOpt
{
    Q_OBJECT

protected:
     void showEvent(QShowEvent*);

public slots:
    void btnOkClicked();
    void btnNotesClicked();
public:
    QString mapStrings[6][3];
    QString apiKey;
    MapViewOptDialog(QWidget* parent);
};

//---------------------------------------------------------------------------
#endif
