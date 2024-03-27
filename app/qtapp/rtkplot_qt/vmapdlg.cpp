//---------------------------------------------------------------------------

#include <QColorDialog>
#include <QShowEvent>

#define INHIBIT_RTK_LOCK_MACROS

#include "vmapdlg.h"
#include "plotmain.h"

extern Plot *plot;

//---------------------------------------------------------------------------
QString color2String(const QColor &c);

//---------------------------------------------------------------------------
VecMapDialog::VecMapDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    connect(btnColor1, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor2, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor3, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor4, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor5, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor6, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor7, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor8, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor9, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor10, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor11, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor12, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor1F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor2F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor3F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor4F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor5F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor6F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor7F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor8F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor9F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor10F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor11F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(btnColor12F, SIGNAL(clicked(bool)), this, SLOT(btnColorClicked()));
    connect(cBVisible1, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible2, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible3, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible4, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible5, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible6, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible7, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible8, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible9, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible10, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible11, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(cBVisible12, SIGNAL(clicked(bool)), this, SLOT(visibilityClicked()));
    connect(rBLayer1, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer2, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer3, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer4, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer5, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer6, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer7, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer8, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer9, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer10, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer11, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(rBLayer12, SIGNAL(clicked(bool)), this, SLOT(layerClicked()));
    connect(btnApply, SIGNAL(clicked(bool)), this, SLOT(btnApplyClicked()));
    connect(btnUp, SIGNAL(clicked(bool)), this, SLOT(btnUpClicked()));
    connect(btnDown, SIGNAL(clicked(bool)), this, SLOT(btnDownClicked()));
    connect(btnClose, SIGNAL(clicked(bool)), this, SLOT(reject()));
}
//---------------------------------------------------------------------------
void VecMapDialog::btnColorClicked()
{
    QPushButton *color[]={
        btnColor1, btnColor2, btnColor3, btnColor4, btnColor5, btnColor6, btnColor7, btnColor8, btnColor9,
        btnColor10, btnColor11, btnColor12, btnColor1F, btnColor2F, btnColor3F, btnColor4F, btnColor5F,
        btnColor6F, btnColor7F, btnColor8F, btnColor9F, btnColor10F, btnColor11F, btnColor12F, NULL
    };
    int i;

    for (i=0;color[i];i++) {
        if (color[i]==sender()) break;
    }
    if (color[i]) {
        QColorDialog dialog(this);

        dialog.setCurrentColor(colors[i]);

        if (dialog.exec() != QDialog::Accepted) return;

        color[i]->setStyleSheet(QString("QPushButton {background-color: %1;}").arg(color2String(dialog.currentColor())));
        colors[i] = dialog.currentColor();

        updateMap();
    }
}
//---------------------------------------------------------------------------
void VecMapDialog::layerClicked()
{
    QRadioButton *layer[] = {
        rBLayer1, rBLayer2,  rBLayer3, rBLayer4, rBLayer5, rBLayer6, rBLayer7, rBLayer8, rBLayer9,
        rBLayer10, rBLayer11, rBLayer12
	};

    for (int i = 0; i < MAXMAPLAYER; i++)
        layer[i]->setChecked((layer[i] == (QRadioButton *)sender()));

    updateMap();
}
//---------------------------------------------------------------------------
void VecMapDialog::btnUpClicked()
{
    QRadioButton *layer[] = {
        rBLayer1, rBLayer2,  rBLayer3, rBLayer4, rBLayer5, rBLayer6, rBLayer7, rBLayer8, rBLayer9,
        rBLayer10, rBLayer11, rBLayer12
	};
    QPushButton *color[]={
               btnColor1, btnColor2, btnColor3, btnColor4, btnColor5, btnColor6, btnColor7, btnColor8, btnColor9,
               btnColor10, btnColor11, btnColor12
    };
    QString style;
    QPushButton *colorf[]={
        btnColor1F, btnColor2F, btnColor3F, btnColor4F, btnColor5F, btnColor6F, btnColor7F, btnColor8F,
        btnColor9F, btnColor10F, btnColor11F, btnColor12F
    };
	gisd_t *data;
	char name[256];
    int i, flag;

    for (i = 0; i < MAXMAPLAYER; i++)
        if (layer[i]->isChecked()) break;
    if (i == 0 || i >= MAXMAPLAYER) return;
    strcpy(name, gis.name[i - 1]);
    strcpy(gis.name[i - 1], gis.name[i]);
    strcpy(gis.name[i], name);
    flag = gis.flag[i - 1];
    gis.flag[i - 1] = gis.flag[i];
    gis.flag[i] = flag;
    data = gis.data[i - 1];
    gis.data[i - 1] = gis.data[i];
    gis.data[i] = data;
    style=color[i-1]->styleSheet();
    color[i-1]->setStyleSheet(color[i]->styleSheet());
    color[i]->setStyleSheet(style);
    style=colorf[i-1]->styleSheet();
    colorf[i-1]->setStyleSheet(colorf[i]->styleSheet());
    colorf[i]->setStyleSheet(style);
    layer[i - 1]->setChecked(true);
    layer[i]->setChecked(false);
    updateMap();
}
//---------------------------------------------------------------------------
void VecMapDialog::btnDownClicked()
{
    QRadioButton *layer[] = {
        rBLayer1, rBLayer2, rBLayer3, rBLayer4, rBLayer5, rBLayer6, rBLayer7, rBLayer8, rBLayer9,
        rBLayer10, rBLayer11, rBLayer12
	};
    QPushButton *color[]={
        btnColor1, btnColor2, btnColor3, btnColor4, btnColor5, btnColor6, btnColor7, btnColor8, btnColor9,
        btnColor10, btnColor11, btnColor12
    };
    QString style;
    QPushButton *colorf[]={
        btnColor1F, btnColor2F, btnColor3F, btnColor4F, btnColor5F, btnColor6F, btnColor7F, btnColor8F,
        btnColor9F, btnColor10F, btnColor11F, btnColor12F
    };
	gisd_t *data;
	char name[256];
    int i, flag;

    for (i = 0; i < MAXMAPLAYER; i++)
        if (layer[i]->isChecked()) break;
    if (i == MAXMAPLAYER - 1 || i >= MAXMAPLAYER) return;
    strcpy(name, gis.name[i + 1]);
    strcpy(gis.name[i + 1], gis.name[i]);
    strcpy(gis.name[i], name);
    flag = gis.flag[i + 1];
    gis.flag[i + 1] = gis.flag[i];
    gis.flag[i] = flag;
    data = gis.data[i + 1];
    gis.data[i + 1] = gis.data[i];
    gis.data[i] = data;
    style=color[i+1]->styleSheet();
    color[i+1]->setStyleSheet(color[i]->styleSheet());
    color[i]->setStyleSheet(style);
    style=colorf[i+1]->styleSheet();
    colorf[i+1]->setStyleSheet(colorf[i]->styleSheet());
    colorf[i]->setStyleSheet(style);

    layer[i + 1]->setChecked(true);
    layer[i]->setChecked(false);
    updateMap();
}
//---------------------------------------------------------------------------
void VecMapDialog::visibilityClicked()
{
    QCheckBox *vis[] = {
        cBVisible1, cBVisible2, cBVisible3, cBVisible4, cBVisible5, cBVisible6, cBVisible7, cBVisible8, cBVisible9, cBVisible10, cBVisible11, cBVisible12
	};

    for (int i = 0; i < MAXMAPLAYER; i++)
        if (qobject_cast<QCheckBox *>(sender()) == vis[i]) gis.flag[i] = vis[i]->isChecked() ? 1 : 0;

    updateMap();
}
//---------------------------------------------------------------------------
void VecMapDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    QRadioButton *layer[] = {
        rBLayer1, rBLayer2, rBLayer3, rBLayer4, rBLayer5, rBLayer6, rBLayer7, rBLayer8, rBLayer9,
        rBLayer10, rBLayer11, rBLayer12
	};
    QPushButton *color[] = {
        btnColor1, btnColor2, btnColor3, btnColor4, btnColor5, btnColor6, btnColor7, btnColor8, btnColor9,
        btnColor10, btnColor11, btnColor12
	};
    QPushButton *colorf[]={
        btnColor1F, btnColor2F, btnColor3F, btnColor4F, btnColor5F, btnColor6F, btnColor7F, btnColor8F,
        btnColor9F, btnColor10F, btnColor11F, btnColor12F
    };
    gis = plot->gis;
    for (int i = 0; i < MAXMAPLAYER; i++) {
        layer[i]->setChecked(false);
        color[i]->setStyleSheet(QString("QPushButton {background-color: %1;}").arg(color2String(plot->mapColor[i])));
        colorf[i]->setStyleSheet(QString("QPushButton {background-color: %1;}").arg(color2String(plot->mapColor[i])));
	}
    updateMap();
}
//---------------------------------------------------------------------------
void VecMapDialog::btnApplyClicked()
{

    updateMap();

    accept();
}
//---------------------------------------------------------------------------
void VecMapDialog::updateMap(void)
{
    QRadioButton *layer[] = {
        rBLayer1, rBLayer2, rBLayer3, rBLayer4, rBLayer5, rBLayer6, rBLayer7, rBLayer8, rBLayer9,
        rBLayer10, rBLayer11, rBLayer12
	};
    QCheckBox *vis[] = {
        cBVisible1, cBVisible2, cBVisible3, cBVisible4, cBVisible5, cBVisible6, cBVisible7, cBVisible8, cBVisible9, cBVisible10, cBVisible11, cBVisible12
	};

    for (int i = 0; i < MAXMAPLAYER; i++) {
        layer[i]->setText(gis.name[i]);
        vis[i]->setChecked(gis.flag[i]);
        plot->mapColor [i]=colors[i];
        plot->mapColorF[i]=colors[i+12];
    }
    plot->gis = gis;
}
//---------------------------------------------------------------------------
