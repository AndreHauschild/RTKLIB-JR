#include "helper.h"


QString color2String(const QColor &c)
{
    return QString("rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue());
}

