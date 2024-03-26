#include "helper.h"

#include <math.h>

#include <QLabel>


//---------------------------------------------------------------------------
QString color2String(const QColor &c)
{
    return QString("rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue());
}
//---------------------------------------------------------------------------
void setWidgetBackgroundColor(QWidget * label, const QColor &color)
{
    // use palett instead of stylesheet here for speed reasons
    if (true) {
        label->setAutoFillBackground(true);
        QPalette palette = label->palette();
        palette.setColor(label->backgroundRole(), color);
        label->setPalette(palette);
    } else
        label->setStyleSheet(QString("QLabel {background-color: %1}").arg(color2String(color)));
}
//---------------------------------------------------------------------------
void setWidgetTextColor(QWidget * label, const QColor &color)
{
    // use palett instead of stylesheet here for speed reasons
    if (true) {
        QPalette palette = label->palette();
        palette.setColor(label->foregroundRole(), color);
        label->setPalette(palette);
    } else
        label->setStyleSheet(QString("QLabel {color: %1}").arg(color2String(color)));
}

// convert degree to deg-min-sec --------------------------------------------
void degtodms(double deg, double *dms)
{
    double sgn = 1.0;

    if (deg < 0.0) {
        deg = -deg;
        sgn = -1.0;
    }
    dms[0] = floor(deg);
    dms[1] = floor((deg - dms[0]) * 60.0);
    dms[2] = (deg - dms[0] - dms[1] / 60.0) * 3600;
    dms[0] *= sgn;
}
