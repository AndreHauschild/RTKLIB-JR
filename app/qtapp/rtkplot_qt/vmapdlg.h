//---------------------------------------------------------------------------

#ifndef vmapdlgH
#define vmapdlgH
//---------------------------------------------------------------------------
#include <QDialog>

#include "rtklib.h"
#include "ui_vmapdlg.h"

//---------------------------------------------------------------------------
class VecMapDialog : public QDialog, private Ui::VecMapDialog
{
    Q_OBJECT
public slots:
    void btnColorClicked();
    void btnApplyClicked();
    void visibilityClicked();
    void layerClicked();
    void btnUpClicked();
    void btnDownClicked();

protected:
    void showEvent (QShowEvent *event);

private:
    gis_t gis;
    QColor colors[12];

    void updateMap(void);

public:
    explicit VecMapDialog(QWidget *parent);
};
//---------------------------------------------------------------------------
#endif
