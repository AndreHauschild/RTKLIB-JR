//---------------------------------------------------------------------------
// plotdraw.c: rtkplot draw functions
//---------------------------------------------------------------------------
#include <QColor>
#include <QPainter>
#include <QDebug>

#define INHIBIT_RTK_LOCK_MACROS
#include "rtklib.h"
#include "plotmain.h"
#include "graph.h"
#include "mapview.h"


#define MS_FONT     "Consolas"      // monospace font name

#define COL_ELMASK  Qt::red
#define ATAN2(x, y)  ((x) * (x) + (y) * (y) > 1E-12 ? atan2(x, y) : 0.0)

// update plot --------------------------------------------------------------
void Plot::updatePlot(void)
{
    trace(3, "UpdatePlot\n");

    updateInfo();
    refresh();
}
// refresh plot -------------------------------------------------------------
void Plot::refresh(void)
{
    trace(3, "Refresh\n");

    flush = 1;
    updateDisplay();
    lblDisplay->update();
}
// draw plot ----------------------------------------------------------------
void Plot::updateDisplay(void)
{
    int level = Drag ? 0 : 1;

    trace(3, "UpdateDisp\n");

    if (flush) {
        buffer = QPixmap(lblDisplay->size());
        if (buffer.isNull()) return;
        buffer.fill(cColor[0]);

        QPainter c(&buffer);

        c.setFont(lblDisplay->font());
        c.setPen(cColor[0]);
        c.setBrush(cColor[0]);

        switch (plotType) {
            case  PLOT_TRK: drawTrack(c, level);   break;
            case  PLOT_SOLP: drawSolution(c, level, 0); break;
            case  PLOT_SOLV: drawSolution(c, level, 1); break;
            case  PLOT_SOLA: drawSolution(c, level, 2); break;
            case  PLOT_NSAT: drawNsat(c, level);   break;
            case  PLOT_OBS: drawObservation(c, level);   break;
            case  PLOT_SKY: drawSky(c, level);   break;
            case  PLOT_DOP: drawDop(c, level);   break;
            case  PLOT_RES: drawResidual(c, level);   break;
            case  PLOT_RESE: drawResidualE(c, level);   break;
            case  PLOT_SNR: drawSnr(c, level);   break;
            case  PLOT_SNRE: drawSnrE(c, level);   break;
            case  PLOT_MPS: drawMpS(c, level);   break;
        }

        lblDisplay->setPixmap(buffer);
    }

    flush = 0;
}
// draw track-plot ----------------------------------------------------------
void Plot::drawTrack(QPainter &c, int level)
{
    QString header;
    TIMEPOS *pos, *pos1, *pos2, *vel;
    gtime_t time1 = { 0, 0 }, time2 = { 0, 0 };
    sol_t *sol;
    QPoint p1, p2;
    double xt, yt, sx, sy, opos[3], pnt[3], rr[3], enu[3]={}, cent[3];
    int i, sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0, p = 0;

    trace(3, "DrawTrk: level=%d\n", level);

    if (btnShowTrack->isChecked() && btnFixCenter->isChecked()) {
        if (!btnSolution12->isChecked()) {
            pos = solutionToPosition(solutionData + sel, solutionIndex[sel], 0, 0);

            if (pos->n > 0) graphT->setCenter(pos->x[0], pos->y[0]);

            delete pos;
        } else {
            pos1 = solutionToPosition(solutionData, solutionIndex[0], 0, 0);
            pos2 = solutionToPosition(solutionData + 1, solutionIndex[1], 0, 0);
            pos = pos1->diff(pos2, 0);

            if (pos->n > 0) graphT->setCenter(pos->x[0], pos->y[0]);

            delete pos;
            delete pos1;
            delete pos2;
        }
    }
    if (!btnSolution12->isChecked() && btnShowImage->isChecked())  // image
        drawTrackImage(c, level);

    if (btnShowMap->isChecked())                            // map
        drawTrackMap(c, level);

    if (btnShowGrid->isChecked()) { // grid
        if (level) { // center +
            graphT->getPosition(p1, p2);
            p1.setX((p1.x() + p2.x()) / 2);
            p1.setY((p1.y() + p2.y()) / 2);
            drawMark(graphT, c, p1, 5, cColor[1], 20, 0);
        }
        if (showGridLabel >= 3) { // circles
            graphT->xLabelPosition = 7; graphT->yLabelPosition = 7;
            graphT->drawCircles(c, showGridLabel == 4);
        }
        else if (showGridLabel >= 1) { // grid
            graphT->xLabelPosition = 2; graphT->yLabelPosition = 4;
            graphT->drawAxis(c, showLabel, showGridLabel == 2);
        }
    }

    if (norm(oPosition, 3) > 0.0) {
        ecef2pos(oPosition, opos);
        header = "ORI=" + latLonString(opos, 9) + QString(" %1m").arg(opos[2], 0, 'f', 4);
    }

    if (btnSolution1->isChecked()) {
        pos = solutionToPosition(solutionData, -1, cBQFlag->currentIndex(), 0);
        drawTrackPoint(c, pos, level, 0);
        if (btnShowMap->isChecked() && norm(solutionData[0].rb, 3) > 1E-3)
            drawTrackPosition(c, solutionData[0].rb, 0, 8, cColor[2], tr("Base Station 1"));
        drawTrackStat(c, pos, header, p++);
        header = "";
        delete pos;
    }

    if (btnSolution2->isChecked() && norm(solutionData[1].rb, 3) > 1E-3) {
        pos = solutionToPosition(solutionData + 1, -1, cBQFlag->currentIndex(), 0);
        drawTrackPoint(c, pos, level, 1);
        if (btnShowMap->isChecked())
            drawTrackPosition(c, solutionData[1].rb, 0, 8, cColor[2], tr("Base Station 2"));
        drawTrackStat(c, pos, header, p++);
        delete pos;
    }

    if (btnSolution12->isChecked()) {
        pos1 = solutionToPosition(solutionData, -1, 0, 0);
        pos2 = solutionToPosition(solutionData + 1, -1, 0, 0);
        pos = pos1->diff(pos2, cBQFlag->currentIndex());
        drawTrackPoint(c, pos, level, 0);
        drawTrackStat(c, pos, "", p++);
        delete pos;
        delete pos1;
        delete pos2;
    }

    if (btnShowTrack->isChecked() && btnSolution1->isChecked()) {
        pos = solutionToPosition(solutionData, solutionIndex[0], 0, 0);

        if ((sol = getsol(solutionData, solutionIndex[0]))) time1 = sol->time;

        if (pos->n) {
            pos->n = 1;
            drawTrackError(c, pos, 0);
            graphT->toPoint(pos->x[0], pos->y[0], p1);
            graphT->drawMark(c, p1, 0, cColor[0], markSize * 2 + 12, 0);
            graphT->drawMark(c, p1, 1, cColor[2], markSize * 2 + 10, 0);
            graphT->drawMark(c, p1, 5, cColor[2], markSize * 2 + 14, 0);
            graphT->drawMark(c, p1, 0, cColor[2], markSize * 2 + 6, 0);
            graphT->drawMark(c, p1, 0, mColor[0][pos->q[0]], markSize * 2 + 4, 0);

            if (btnSolution2->isChecked()) {
                p1.rx() += markSize + 8;
                drawLabel(graphT, c, p1, "1", 1, 0);
            }
        }
        delete pos;
    }

    if (btnShowTrack->isChecked() && btnSolution2->isChecked()) {
        pos = solutionToPosition(solutionData + 1, solutionIndex[1], 0, 0);

        if ((sol = getsol(solutionData + 1, solutionIndex[1]))) time2 = sol->time;

        if (pos->n > 0 && (time1.time == 0 || fabs(timediff(time1, time2)) < DTTOL * 2.0)) {
            pos->n = 1;
            drawTrackError(c, pos, 1);
            graphT->toPoint(pos->x[0], pos->y[0], p1);
            graphT->drawMark(c, p1, 0, cColor[0], markSize * 2 + 12, 0);
            graphT->drawMark(c, p1, 1, cColor[1], markSize * 2 + 10, 0);
            graphT->drawMark(c, p1, 5, cColor[1], markSize * 2 + 14, 0);
            graphT->drawMark(c, p1, 0, cColor[2], markSize * 2 + 6, 0);
            graphT->drawMark(c, p1, 0, mColor[1][pos->q[0]], markSize * 2 + 4, 0);

            if (btnSolution1->isChecked()) {
                p1.setX(p1.x() + markSize + 8);
                drawLabel(graphT, c, p1, "2", 1, 0);
            }
        }
        delete pos;
    }
    if (btnShowTrack->isChecked() && btnSolution12->isChecked()) {
        pos1 = solutionToPosition(solutionData, solutionIndex[0], 0, 0);
        pos2 = solutionToPosition(solutionData + 1, solutionIndex[1], 0, 0);
        pos = pos1->diff(pos2, 0);

        if (pos->n > 0) {
            pos->n = 1;
            drawTrackError(c, pos, 1);
            graphT->toPoint(pos->x[0], pos->y[0], p1);
            graphT->drawMark(c, p1, 0, cColor[0], markSize * 2 + 12, 0);
            graphT->drawMark(c, p1, 1, cColor[2], markSize * 2 + 10, 0);
            graphT->drawMark(c, p1, 5, cColor[2], markSize * 2 + 14, 0);
            graphT->drawMark(c, p1, 0, cColor[2], markSize * 2 + 6, 0);
            graphT->drawMark(c, p1, 0, mColor[0][pos->q[0]], markSize * 2 + 4, 0);
        }
        delete pos;
        delete pos1;
        delete pos2;
    }
    if (level&& btnShowMap->isChecked()) {
        for (i = 0; i < nWayPoint; i++) {
            if (i==selectedWayPoint) continue;
            pnt[0] = pointPosition[i][0] * D2R;
            pnt[1] = pointPosition[i][1] * D2R;
            pnt[2] = pointPosition[i][2];
            pos2ecef(pnt, rr);
            drawTrackPosition(c, rr, 0, 8, cColor[2], pointName[i]);
        }
        if (selectedWayPoint>=0) {
            pnt[0]=pointPosition[selectedWayPoint][0]*D2R;
            pnt[1]=pointPosition[selectedWayPoint][1]*D2R;
            pnt[2]=pointPosition[selectedWayPoint][2];
            pos2ecef(pnt, rr);
            drawTrackPosition(c,rr, 0, 16, Qt::red, pointName[selectedWayPoint]);
        }
    }

    if (showCompass) {
        graphT->getPosition(p1, p2);
        p1.rx() += SIZE_COMP / 2 + 25;
        p1.ry() += SIZE_COMP / 2 + 35;
        drawMark(graphT, c, p1, 13, cColor[2], SIZE_COMP, 0);
    }

    if (showArrow && btnShowTrack->isChecked()) {
        vel = solutionToPosition(solutionData + sel, solutionIndex[sel], 0, 1);
        drawTrackVelocity(c, vel);
        delete vel;
    }

    if (showScale) {
        QString label;
        graphT->getPosition(p1, p2);
        graphT->getTick(xt, yt);
        graphT->getScale(sx, sy);
        p2.rx() -= 70;
        p2.ry() -= 25;
        drawMark(graphT, c, p2, 11, cColor[2], static_cast<int>(xt / sx + 0.5), 0);
        p2.ry() -= 3;
        if (xt < 0.0099) label = QString("%1 mm").arg(xt * 1000.0, 0, 'f', 0);
        else if (xt < 0.999) label = QString("%1 cm").arg(xt * 100.0, 0, 'f', 0);
        else if (xt < 999.0) label = QString("%1 m").arg(xt, 0, 'f', 0);
        else label = QString("%1 km").arg(xt / 1000.0, 0, 'f', 0);
        drawLabel(graphT, c, p2, label, 0, 1);
    }

    if (!level) { // center +
        graphT->getCenter(xt, yt);
        graphT->toPoint(xt, yt, p1);
        drawMark(graphT, c, p1, 5, cColor[2], 20, 0);
    }

    // update map center
    if (level) {
        if (norm(oPosition, 3) > 0.0) {
            graphT->getCenter(xt, yt);
            graphT->toPoint(xt, yt, p1);
            graphT->toPos(p1, enu[0], enu[1]);
            ecef2pos(oPosition, opos);
            enu2ecef(opos, enu, rr);
            for (i=0; i<3; i++) rr[i] += oPosition[i];
            ecef2pos(rr, cent);
            mapView->setCenter(cent[0] * R2D, cent[1] * R2D);
        }
        refresh_MapView();
    }
}
// draw map-image on track-plot ---------------------------------------------
void Plot::drawTrackImage(QPainter &c, int level)
{
    gtime_t time = { 0, 0 };
    QPoint p1, p2;
    double pos[3] = { 0 }, rr[3], xyz[3] = { 0 }, x1, x2, y1, y2;

    trace(3, "DrawTrkImage: level=%d\n", level);

    pos[0] = mapLatitude * D2R;
    pos[1] = mapLongitude * D2R;
    pos2ecef(pos, rr);
    if (norm(oPosition, 3) > 0.0)
        positionToXyz(time, rr, 0, xyz);
    x1 = xyz[0] - mapSize[0] * 0.5 * mapScaleX;
    x2 = xyz[0] + mapSize[0] * 0.5 * mapScaleX;
    y1 = xyz[1] - mapSize[1] * 0.5 * (mapScaleEqual ? mapScaleX : mapScaleY);
    y2 = xyz[1] + mapSize[1] * 0.5 * (mapScaleEqual ? mapScaleX : mapScaleY);

    graphT->toPoint(x1, y2, p1);
    graphT->toPoint(x2, y1, p2);
    QRect r(p1, p2);
    c.drawImage(r, mapImage);
}
// check in boundary --------------------------------------------------------
#define P_IN_B(pos, bound) \
    (pos[0] >= bound[0] && pos[0] <= bound[1] && pos[1] >= bound[2] && pos[1] <= bound[3])

#define B_IN_B(bound1, bound2) \
    (bound1[0] <= bound2[1] && bound1[1] >= bound2[0] && bound1[2] <= bound2[3] && bound1[3] >= bound2[2])

// draw gis-map on track-plot ----------------------------------------------
void Plot::drawTrackMap(QPainter &c, int level)
{
    gisd_t *data;
    gis_pnt_t *pnt;
    gis_poly_t *poly;
    gis_polygon_t *polygon;
    gtime_t time = { 0, 0 };
    QColor color;
    QPoint *p, p1;
    double xyz[3], S, xl[2], yl[2], enu[8][3] = { { 0 } }, opos[3], pos[3], rr[3];
    double bound[4] = { PI / 2.0, -PI / 2.0, PI, -PI };
    int i, j, n, m;

    trace(3, "DrawTrkPath: level=%d\n", level);

    // get map boundary
    graphT->getLimits(xl, yl);
    enu[0][0] = xl[0]; enu[0][1] = yl[0];
    enu[1][0] = xl[1]; enu[1][1] = yl[0];
    enu[2][0] = xl[0]; enu[2][1] = yl[1];
    enu[3][0] = xl[1]; enu[3][1] = yl[1];
    enu[4][0] = (xl[0] + xl[1]) / 2.0; enu[4][1] = yl[0];
    enu[5][0] = (xl[0] + xl[1]) / 2.0; enu[5][1] = yl[1];
    enu[6][0] = xl[0]; enu[6][1] = (yl[0] + yl[1]) / 2.0;
    enu[7][0] = xl[1]; enu[7][1] = (yl[0] + yl[1]) / 2.0;

    ecef2pos(oPosition, opos);

    for (i = 0; i < 8; i++) {
        if (norm(enu[i], 2) >= 1000000.0) {
            bound[0] =-PI / 2.0;
            bound[1] = PI / 2.0;
            bound[2] =-PI;
            bound[3] = PI;
            break;
        }
        enu2ecef(opos, enu[i], rr);
        for (j = 0; j < 3; j++) rr[j] += oPosition[j];
        ecef2pos(rr, pos);
        if (pos[0] < bound[0]) bound[0] = pos[0];       // min lat
        if (pos[0] > bound[1]) bound[1] = pos[0];       // max lat
        if (pos[1] < bound[2]) bound[2] = pos[1];       // min lon
        if (pos[1] > bound[3]) bound[3] = pos[1];       // max lon
    }

    for (i = MAXMAPLAYER - 1; i >= 0; i--) {
        if (!gis.flag[i]) continue;
        for (data = gis.data[i]; data; data = data->next) {
            if (data->type == 1) { // point
                pnt = static_cast<gis_pnt_t *>(data->data);
                if (!P_IN_B(pnt->pos, bound)) continue;
                positionToXyz(time, pnt->pos, 0, xyz);
                if (xyz[2]<-RE_WGS84) continue;
                graphT->toPoint(xyz[0], xyz[1], p1);
                drawMark(graphT, c, p1, 1, cColor[2], 6, 0);
                drawMark(graphT, c, p1, 0, cColor[2], 2, 0);
            } else if (level && data->type == 2) { // polyline
                poly = static_cast<gis_poly_t *>(data->data);
                if ((n = poly->npnt) <= 0 || !B_IN_B(poly->bound, bound))
                    continue;
                p = new QPoint [n];
                for (j = m = 0; j < n; j++) {
                    positionToXyz(time, poly->pos + j * 3, 0, xyz);
                    if (xyz[2] < -RE_WGS84) {
                        if (m > 1) {
                            graphT->drawPoly(c, p, m, mapColor[i], 0);
                            m = 0;
                        }
                        continue;
                    }
                    graphT->toPoint(xyz[0], xyz[1], p1);
                    if (m == 0 || p1.x() != p[m - 1].x() || p1.y() != p[m - 1].y())
                        p[m++] = p1;
                }
                graphT->drawPoly(c, p, m, mapColor[i], 0);
                delete [] p;
            } else if (level && data->type == 3) { // polygon
                polygon = (gis_polygon_t *)data->data;
                if ((n = polygon->npnt) <= 0 || !B_IN_B(polygon->bound, bound))
                    continue;
                p = new QPoint [n];
                for (j = m = 0; j < n; j++) {
                    positionToXyz(time, polygon->pos + j * 3, 0, xyz);
                    if (xyz[2] < -RE_WGS84) {
                        continue;
                    }
                    graphT->toPoint(xyz[0], xyz[1], p1);
                    if (m == 0 || p1.x() != p[m - 1].x() || p1.y() != p[m - 1].y())
                        p[m++] = p1;
                }
                            // judge hole
                for (j = 0, S = 0.0; j < m - 1; j++)
                    S += static_cast<double>(p[j].x() * p[j + 1].y() - p[j + 1].x() * p[j].y());
                color = S < 0.0 ? cColor[0] : mapColorF[i];
                graphT->drawPatch(c, p, m, mapColor[i], color, 0);
                delete [] p;
            }
        }
    }
}
// draw track-points on track-plot ------------------------------------------
void Plot::drawTrackPoint(QPainter &c, const TIMEPOS *pos, int level, int style)
{
    QColor* color = new QColor[pos->n];
    int i;

    trace(3, "DrawTrkPnt: level=%d style=%d\n", level, style);

    if (level) drawTrackArrow(c, pos);

    if (level && plotStyle <= 1 && !btnShowTrack->isChecked()) // error circle
        drawTrackError(c, pos, style);

    if (!(plotStyle % 2))
        graphT->drawPoly(c, pos->x, pos->y, pos->n, cColor[3], style);

    if (level && plotStyle < 2) {
        if (btnShowImage->isChecked()) {
            for (i = 0; i < pos->n; i++) color[i] = cColor[0];
            graphT->drawMarks(c, pos->x, pos->y, color, pos->n, 0, markSize + 2, 0);
        }

        for (i = 0; i < pos->n; i++) color[i] = mColor[style][pos->q[i]];

        graphT->drawMarks(c, pos->x, pos->y, color, pos->n, 0, markSize, 0);
    }

    delete[] color;
}
// draw point with label on track-plot --------------------------------------
void Plot::drawTrackPosition(QPainter &c, const double *rr, int type, int siz,
              QColor color, const QString &label)
{
    gtime_t time = { 0, 0 };
    QPoint p1;
    double xyz[3], xs, ys;

    trace(3,"DrawTrkPos: type=%d rr=%.3f %.3f %.3f\n",type,rr[0],rr[1],rr[2]);

    if (norm(rr, 3) > 0.0) {
        graphT->getScale(xs, ys);
        positionToXyz(time, rr, type, xyz);
        graphT->toPoint(xyz[0], xyz[1], p1);
        drawMark(graphT, c, p1, 5, color, siz + 6, 0);
        drawMark(graphT, c, p1, 1, color, siz, 0);
        drawMark(graphT, c, p1, 1, color, siz - 6, 0);
        p1.ry() += 10;
        drawLabel(graphT, c, p1, label, 0, 2);
    }
}
// draw statistics on track-plot --------------------------------------------
void Plot::drawTrackStat(QPainter &c, const TIMEPOS *pos, const QString &header, int p)
{
    QString s[6];
    QPoint p1, p2;
    double *d, ave[4], std[4], rms[4];
    int i, n = 0, fonth = static_cast<int>(lblDisplay->font().pointSize() * 1.5);

    trace(3, "DrawTrkStat: p=%d\n", p);

    if (!showStats) return;

    if (p == 0 && header != "") s[n++] = header;

    if (pos->n > 0) {
        d = new double[pos->n];

        for (i = 0; i < pos->n; i++)
            d[i] = SQRT(SQR(pos->x[i]) + SQR(pos->y[i]));

        calcStats(pos->x, pos->n, 0.0, ave[0], std[0], rms[0]);
        calcStats(pos->y, pos->n, 0.0, ave[1], std[1], rms[1]);
        calcStats(pos->z, pos->n, 0.0, ave[2], std[2], rms[2]);
        calcStats(d, pos->n, 0.0, ave[3], std[3], rms[3]);

        s[n++] = QString("AVE=E:%1m N:%2m U:%3m").arg(ave[0], 7, 'f', 4).arg(ave[1], 7, 'f', 4).arg(ave[2], 7, 'f', 4);
        s[n++] = QString("STD=E:%1m N:%2m U:%3m").arg(std[0], 7, 'f', 4).arg(std[1], 7, 'f', 4).arg(std[2], 7, 'f', 4);
        s[n++] = QString("RMS=E:%1m N:%2m U:%3m 2D:%4m")
             .arg(rms[0], 7, 'f', 4).arg(rms[1], 7, 'f', 4).arg(rms[2], 7, 'f', 4).arg(2.0 * rms[3], 7, 'f', 4);

        delete [] d;
    }
    graphT->getPosition(p1, p2);
    p1.rx() = p2.x() - 10;
    p1.ry() += 8 + fonth * 4 * p;

    for (i = 0; i < n; i++, p1.ry() += fonth)
        drawLabel(graphT, c, p1, s[i], 2, 2);
}
// draw error-circle on track-plot ------------------------------------------
void Plot::drawTrackError(QPainter &c, const TIMEPOS *pos, int style)
{
    const double sint[36] = {
        0.0000,	 0.1736,  0.3420,  0.5000,  0.6428,  0.7660,  0.8660,  0.9397,	0.9848,
        1.0000,	 0.9848,  0.9397,  0.8660,  0.7660,  0.6428,  0.5000,  0.3420,	0.1736,
        0.0000,	 -0.1736, -0.3420, -0.5000, -0.6428, -0.7660, -0.8660, -0.9397, -0.9848,
        -1.0000, -0.9848, -0.9397, -0.8660, -0.7660, -0.6428, -0.5000, -0.3420, -0.1736
    };
    double xc[37], yc[37], a, b, s, cc;
    int i, j;

    trace(3, "DrawTrkError: style=%d\n", style);

    if (!showError) return;

    for (i = 0; i < pos->n; i++) {
        if (pos->xs[i] <= 0.0 || pos->ys[i] <= 0.0) continue;

        a = pos->xys[i] / SQRT(pos->xs[i]);

        if ((b = pos->ys[i] - a * a) >= 0.0) b = SQRT(b); else continue;

        for (j = 0; j < 37; j++) {
            s = sint[j % 36];
            cc = sint[(45 - j) % 36];
            xc[j] = pos->x[i] + SQRT(pos->xs[i]) * cc;
            yc[j] = pos->y[i] + a * cc + b * s;
        }
        graphT->drawPoly(c, xc, yc, 37, cColor[1], showError == 1 ? 0 : 1);
    }
}
// draw direction-arrow on track-plot ---------------------------------------
void Plot::drawTrackArrow(QPainter &c, const TIMEPOS *pos)
{
    QPoint p;
    double tt, d[2], dist, dt, vel;
    int i, off = 8;

    trace(3, "DrawTrkArrow\n");

    if (!showArrow) return;

    for (i = 1; i < pos->n - 1; i++) {
        tt = time2gpst(pos->t[i], NULL);
        d[0] = pos->x[i + 1] - pos->x[i - 1];
        d[1] = pos->y[i + 1] - pos->y[i - 1];
        dist = norm(d, 2);
        dt = timediff(pos->t[i + 1], pos->t[i - 1]);
        vel = dt == 0.0 ? 0.0 : dist / dt;

        if (vel < 0.5 || fmod(tt + 0.005, INTARROW) >= 0.01) continue;

        graphT->toPoint(pos->x[i], pos->y[i], p);
        p.rx() -= static_cast<int>(off * d[1] / dist);
        p.ry() -= static_cast<int>(off * d[0] / dist);
        drawMark(graphT, c, p, 10, cColor[3], 15, static_cast<int>(ATAN2(d[1], d[0]) * R2D));
    }
}
// draw velocity-indicator on track-plot ------------------------------------
void Plot::drawTrackVelocity(QPainter &c, const TIMEPOS *vel)
{
    QString label;
    QPoint p1, p2;
    double v = 0.0, dir = 0.0;

    trace(3, "DrawTrkVel\n");

    if (vel && vel->n > 0) {
        if ((v = SQRT(SQR(vel->x[0]) + SQR(vel->y[0]))) > 1.0)
            dir = ATAN2(vel->x[0], vel->y[0]) * R2D;
    }

    graphT->getPosition(p1, p2);
    p1.rx() += SIZE_VELC / 2 + 30;
    p1.ry() = p2.y() - SIZE_VELC / 2 - 30;
    drawMark(graphT, c, p1, 1, cColor[2], SIZE_VELC, 0);

    p1.ry() += SIZE_VELC / 2;
    label = QString("%1 km/h").arg(v * 3600.0 / 1000.0, 0, 'f', 0);
    drawLabel(graphT, c, p1, label, 0, 2);

    p1.ry() -= SIZE_VELC / 2;
    if (v >= 1.0) drawMark(graphT, c, p1, 10, cColor[2], SIZE_VELC, 90 - static_cast<int>(dir));
    drawMark(graphT, c, p1, 0, cColor[0], 8, 0);
    drawMark(graphT, c, p1, 1, cColor[2], 8, 0);
}
// draw solution-plot -------------------------------------------------------
void Plot::drawSolution(QPainter &c, int level, int type)
{
    QString label[] = { tr("E-W"), tr("N-S"), tr("U-D") }, unit[] = { "m", "m/s", QString("m/s%1").arg(up2Char) };
    QPushButton *btn[] = { btnOn1, btnOn2, btnOn3 };
    TIMEPOS *pos, *pos1, *pos2;
    gtime_t time1 = { 0, 0 }, time2 = { 0, 0 };
    QPoint p1, p2;
    double xc, yc, xl[2], yl[2], off, y;
    int i, j, k, sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0, p = 0;

    trace(3, "DrawSol: level=%d\n", level);

    if (btnShowTrack->isChecked() && (btnFixHorizontal->isChecked() || btnFixVertical->isChecked())) {
        pos = solutionToPosition(solutionData + sel, solutionIndex[sel], 0, type);

        for (i = 0; i < 3 && pos->n > 0; i++) {
            graphG[i]->getCenter(xc, yc);
            if (btnFixVertical->isChecked())
                yc = i == 0 ? pos->x[0] : (i == 1 ? pos->y[0] : pos->z[0]);

            if (btnFixHorizontal->isChecked()) {
                graphG[i]->getLimits(xl, yl);
                off = Xcent * (xl[1] - xl[0]) / 2.0;
                graphG[i]->setCenter(timePosition(pos->t[0]) - off, yc);
            } else {
                graphG[i]->setCenter(xc, yc);
            }
        }
        delete pos;
    }
    j = -1;

    for (i = 0; i < 3; i++) if (btn[i]->isChecked()) j = i;

    for (i = 0; i < 3; i++) {
        if (!btn[i]->isChecked()) continue;
        graphG[i]->xLabelPosition = timeLabel ? (i == j ? 6 : 5) : (i == j ? 1 : 0);
        graphG[i]->week = week;
        graphG[i]->drawAxis(c, showLabel, showLabel);
    }

    if (btnSolution1->isChecked()) {
        pos = solutionToPosition(solutionData, -1, cBQFlag->currentIndex(), type);
        DdrawSolutionPoint(c, pos, level, 0);
        drawSolutionStat(c, pos, unit[type], p++);

        delete pos;
    }

    if (btnSolution2->isChecked()) {
        pos = solutionToPosition(solutionData + 1, -1, cBQFlag->currentIndex(), type);
        DdrawSolutionPoint(c, pos, level, 1);
        drawSolutionStat(c, pos, unit[type], p++);

        delete pos;
    }

    if (btnSolution12->isChecked()) {
        pos1 = solutionToPosition(solutionData, -1, 0, type);
        pos2 = solutionToPosition(solutionData + 1, -1, 0, type);
        pos = pos1->diff(pos2, cBQFlag->currentIndex());
        DdrawSolutionPoint(c, pos, level, 0);
        drawSolutionStat(c, pos, unit[type], p++);

        delete pos;
        delete pos1;
        delete pos2;
    }

    if (btnShowTrack->isChecked() && (btnSolution1->isChecked() || btnSolution2->isChecked() || btnSolution12->isChecked())) {
        pos = solutionToPosition(solutionData + sel, solutionIndex[sel], 0, type);
        pos1 = solutionToPosition(solutionData, solutionIndex[0], 0, type);
        pos2 = solutionToPosition(solutionData + 1, solutionIndex[1], 0, type);
        if (pos1->n > 0) time1 = pos1->t[0];
        if (pos2->n > 0) time2 = pos2->t[0];

        for (j = k = 0; j < 3 && pos->n > 0; j++) {
            if (!btn[j]->isChecked()) continue;

            graphG[j]->getLimits(xl, yl);
            xl[0] = xl[1] = timePosition(pos->t[0]);
            graphG[j]->drawPoly(c, xl, yl, 2, cColor[2], 0);

            if (btnSolution2->isChecked() && pos2->n > 0 && (time1.time == 0 || fabs(timediff(time1, time2)) < DTTOL * 2.0)) {
                xl[0] = xl[1] = timePosition(time2);
                y = j == 0 ? pos2->x[0] : (j == 1 ? pos2->y[0] : pos2->z[0]);

                graphG[j]->drawMark(c, xl[0], y, 0, cColor[0], markSize * 2 + 6, 0);
                graphG[j]->drawMark(c, xl[0], y, 1, cColor[1], markSize * 2 + 6, 0);
                graphG[j]->drawMark(c, xl[0], y, 1, cColor[2], markSize * 2 + 2, 0);
                graphG[j]->drawMark(c, xl[0], y, 0, mColor[1][pos->q[0]], markSize * 2, 0);

                if (btnSolution1->isChecked() && pos1->n > 0 && graphG[j]->toPoint(xl[0], y, p1)) {
                    p1.rx() += markSize + 4;
                    drawLabel(graphG[j], c, p1, "2", 1, 0);
                }
            }
            if (btnSolution1->isChecked() && pos1->n > 0) {
                xl[0] = xl[1] = timePosition(time1);
                y = j == 0 ? pos1->x[0] : (j == 1 ? pos1->y[0] : pos1->z[0]);

                graphG[j]->drawMark(c, xl[0], y, 0, cColor[0], markSize * 2 + 6, 0);
                graphG[j]->drawMark(c, xl[0], y, 1, cColor[2], markSize * 2 + 6, 0);
                graphG[j]->drawMark(c, xl[0], y, 1, cColor[2], markSize * 2 + 2, 0);
                graphG[j]->drawMark(c, xl[0], y, 0, mColor[0][pos->q[0]], markSize * 2, 0);

                if (btnSolution2->isChecked() && pos2->n > 0 && graphG[j]->toPoint(xl[0], y, p1)) {
                    p1.rx() += markSize + 4;
                    drawLabel(graphG[j], c, p1, "1", 1, 0);
                }
            }
            xl[0] = xl[1] = timePosition(pos->t[0]);
            if (k++ == 0) {
                graphG[j]->drawMark(c, xl[0], yl[1] - 1E-6, 0, cColor[2], 5, 0);

                if (!btnFixHorizontal->isChecked())
                    graphG[j]->drawMark(c, xl[0], yl[1] - 1E-6, 1, cColor[2], 9, 0);
            }
        }

        delete pos;
        delete pos1;
        delete pos2;
    }
    for (i = 0; i < 3; i++) {
        if (!btn[i]->isChecked()) continue;
        graphG[i]->getPosition(p1, p2);
        p1.rx() += 5; p1.ry() += 3;
        drawLabel(graphG[i], c, p1, label[i] + " (" + unit[type] + ")", 1, 2);
    }
}
// draw points and line on solution-plot ------------------------------------
void Plot::DdrawSolutionPoint(QPainter &c, const TIMEPOS *pos, int level, int style)
{
    QPushButton *btn[] = { btnOn1, btnOn2, btnOn3 };
    double *x, *y, *s, xs, ys, *yy;
    int i, j;

    trace(3, "DrawSolPnt: level=%d style=%d\n", level, style);

    x = new double [pos->n];

    for (i = 0; i < pos->n; i++)
        x[i] = timePosition(pos->t[i]);

    for (i = 0; i < 3; i++) {
        if (!btn[i]->isChecked()) continue;

        y = i == 0 ? pos->x : (i == 1 ? pos->y : pos->z);
        s = i == 0 ? pos->xs : (i == 1 ? pos->ys : pos->zs);

        if (!level || !(plotStyle % 2))
            drawPolyS(graphG[i], c, x, y, pos->n, cColor[3], style);

        if (level && showError && plotType <= PLOT_SOLA && plotStyle < 2) {
            graphG[i]->getScale(xs, ys);

            if (showError == 1) {
                for (j = 0; j < pos->n; j++)
                    graphG[i]->drawMark(c, x[j], y[j], 12, cColor[1], static_cast<int>(SQRT(s[j]) * 2.0 / ys), 0);
            } else {
                yy = new double [pos->n];

                for (j = 0; j < pos->n; j++) yy[j] = y[j] - SQRT(s[j]);
                drawPolyS(graphG[i], c, x, yy, pos->n, cColor[1], 1);

                for (j = 0; j < pos->n; j++) yy[j] = y[j] + SQRT(s[j]);
                drawPolyS(graphG[i], c, x, yy, pos->n, cColor[1], 1);

                delete [] yy;
            }
        }
        if (level && plotStyle < 2) {
            QColor * color = new QColor[pos->n];
            for (j = 0; j < pos->n; j++) color[i] = mColor[style][pos->q[j]];
            graphG[i]->drawMarks(c, x, y, color, pos->n, 0, markSize, 0);
            delete[] color;
        }
    }
    delete [] x;
}
// draw statistics on solution-plot -----------------------------------------
void Plot::drawSolutionStat(QPainter &c, const TIMEPOS *pos, const QString &unit, int p)
{
    QPushButton *btn[] = { btnOn1, btnOn2, btnOn3 };
    QPoint p1, p2;
    double ave, std, rms, *y, opos[3];
    int i, j = 0, k = 0, fonth = static_cast<int>(lblDisplay->font().pointSize() * 1.5);
    QString label, s;

    trace(3, "DrawSolStat: p=%d\n", p);

    if (!showStats || pos->n <= 0) return;

    for (i = 0; i < 3; i++) {
        if (!btn[i]->isChecked()) continue;

        y = i == 0 ? pos->x : (i == 1 ? pos->y : pos->z);
        calcStats(y, pos->n, 0.0, ave, std, rms);
        graphG[i]->getPosition(p1, p2);
        p1.rx() = p2.x() - 5;
        p1.ry() += 3 + fonth * (p + (!k++ && p > 0 ? 1 : 0));

        if (j == 0 && p == 0) {
            if (norm(oPosition, 3) > 0.0) {
                ecef2pos(oPosition, opos);
                label = "ORI=" + latLonString(opos, 9) + QString(" %1m").arg(opos[2], 0, 'f', 4);
                drawLabel(graphG[j], c, p1, label, 2, 2);
                j++; p1.ry() += fonth;
            }
        }
        s = QString("AVE=%1%2 STD=%3%2 RMS=%4%2").arg(ave, 0, 'f', 4).arg(unit).arg(std, 0, 'f', 4).arg(rms, 0, 'f', 4);
        drawLabel(graphG[i], c, p1, s, 2, 2);
    }
}
// draw number-of-satellite plot --------------------------------------------
void Plot::drawNsat(QPainter &c, int level)
{
    QString label[] = {
        tr("# of Valid Satellites"),
        tr("Age of Differential (s)"),
        tr("Ratio Factor for AR Validation")
    };
    QPushButton *btn[] = { btnOn1, btnOn2, btnOn3 };
    TIMEPOS *ns;
    QPoint p1, p2;
    double xc, yc, y, xl[2], yl[2], off;
    int i, j, k, sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0;

    trace(3, "DrawNsat: level=%d\n", level);

    if (btnShowTrack->isChecked() && btnFixHorizontal->isChecked()) {
        ns = solutionToNsat(solutionData + sel, solutionIndex[sel], 0);

        for (i = 0; i < 3; i++) {
            if (btnFixHorizontal->isChecked()) {
                graphG[i]->getLimits(xl, yl);
                off = Xcent * (xl[1] - xl[0]) / 2.0;
                graphG[i]->getCenter(xc, yc);
                graphG[i]->setCenter(timePosition(ns->t[0]) - off, yc);
            } else {
                graphG[i]->getRight(xc, yc);
                graphG[i]->setRight(timePosition(ns->t[0]), yc);
            }
        }
        delete ns;
    }
    j = -1;
    for (i = 0; i < 3; i++) if (btn[i]->isChecked()) j = i;
    for (i = 0; i < 3; i++) {
        if (!btn[i]->isChecked()) continue;
        graphG[i]->xLabelPosition = timeLabel ? (i == j ? 6 : 5) : (i == j ? 1 : 0);
        graphG[i]->week = week;
        graphG[i]->drawAxis(c, showLabel, showLabel);
    }

    if (btnSolution1->isChecked()) {
        ns = solutionToNsat(solutionData, -1, cBQFlag->currentIndex());
        DdrawSolutionPoint(c, ns, level, 0);
        delete ns;
    }

    if (btnSolution2->isChecked()) {
        ns = solutionToNsat(solutionData + 1, -1, cBQFlag->currentIndex());
        DdrawSolutionPoint(c, ns, level, 1);
        delete ns;
    }

    if (btnShowTrack->isChecked() && (btnSolution1->isChecked() || btnSolution2->isChecked())) {
        ns = solutionToNsat(solutionData + sel, solutionIndex[sel], 0);

        for (j = k = 0; j < 3 && ns->n > 0; j++) {
            if (!btn[j]->isChecked()) continue;

            y = j == 0 ? ns->x[0] : (j == 1 ? ns->y[0] : ns->z[0]);
            graphG[j]->getLimits(xl, yl);
            xl[0] = xl[1] = timePosition(ns->t[0]);

            graphG[j]->drawPoly(c, xl, yl, 2, cColor[2], 0);
            graphG[j]->drawMark(c, xl[0], y, 0, cColor[0], markSize * 2 + 6, 0);
            graphG[j]->drawMark(c, xl[0], y, 1, cColor[2], markSize * 2 + 6, 0);
            graphG[j]->drawMark(c, xl[0], y, 1, cColor[2], markSize * 2 + 2, 0);
            graphG[j]->drawMark(c, xl[0], y, 0, mColor[sel][ns->q[0]], markSize * 2, 0);

            if (k++ == 0) {
                graphG[j]->drawMark(c, xl[0], yl[1] - 1E-6, 0, cColor[2], 5, 0);

                if (!btnFixHorizontal->isChecked())
                    graphG[j]->drawMark(c, xl[0], yl[1] - 1E-6, 1, cColor[2], 9, 0);
            }
        }
        delete ns;
    }
    for (i = 0; i < 3; i++) {
        if (!btn[i]->isChecked()) continue;
        graphG[i]->getPosition(p1, p2);
        p1.rx() += 5; p1.ry() += 3;
        drawLabel(graphG[i], c, p1, label[i], 1, 2);
    }
}
// draw observation-data-plot -----------------------------------------------
void Plot::drawObservation(QPainter &c, int level)
{
    QPoint p1, p2, p;
    gtime_t time;
    obsd_t *obs;
    double xs, ys, xt, xl[2], yl[2], tt[MAXSAT] = { 0 }, xp, xc, yc, yp[MAXSAT] = { 0 };
    int i, j, m = 0, sats[MAXSAT] = { 0 }, ind = observationIndex;
    char id[16];

    trace(3, "DrawObs: level=%d\n", level);

    for (i = 0; i < observation.n; i++) {
        if (satelliteMask[observation.data[i].sat - 1]) continue;
        sats[observation.data[i].sat - 1] = 1;
    }
    for (i = 0; i < MAXSAT; i++) if (sats[i]) m++;
    
    graphR->xLabelPosition = timeLabel ? 6 : 1;
    graphR->yLabelPosition = 0;
    graphR->week = week;
    graphR->getLimits(xl, yl);
    yl[0] = 0.5;
    yl[1] = m > 0 ? m + 0.5 : m + 10.5;
    graphR->setLimits(xl, yl);
    graphR->setTick(0.0, 1.0);

    if (0 <= ind && ind < nObservation && btnShowTrack->isChecked() && btnFixHorizontal->isChecked()) {
        xp = timePosition(observation.data[indexObservation[ind]].time);
        if (btnFixHorizontal->isChecked()) {
            double xl[2], yl[2], off;
            graphR->getLimits(xl, yl);
            off = Xcent * (xl[1] - xl[0]) / 2.0;
            graphR->getCenter(xc, yc);
            graphR->setCenter(xp - off, yc);
        } else {
            graphR->getRight(xc, yc);
            graphR->setRight(xp, yc);
        }
    }
    graphR->drawAxis(c, 1, 1);
    graphR->getPosition(p1, p2);

    for (i = 0, j = 0; i < MAXSAT; i++) {
        QString label;
        if (!sats[i]) continue;
        p.setX(p1.x());
        p.setY(p1.y() + static_cast<int>((p2.y() - p1.y()) * (j + 0.5) / m));
        yp[i] = m - (j++);
        satno2id(i + 1, id);
        label = id;
        graphR->drawText(c, p, label, cColor[2], 2, 0, 0);
    }
    p1.setX(lblDisplay->font().pointSize());
    p1.setY((p1.y() + p2.y()) / 2);
    graphR->drawText(c, p1, tr("SATELLITE NO"), cColor[2], 0, 0, 90);

    if (!btnSolution1->isChecked()) return;

    if (level && plotStyle <= 2)
        drawObservationEphemeris(c, yp);

    if (level && plotStyle <= 2) {
        graphR->getScale(xs, ys);
        for (i = 0; i < observation.n; i++) {
            obs = &observation.data[i];
            QColor col = observationColor(obs, azimuth[i], elevtion[i]);
            if (col == Qt::black) continue;

            xt = timePosition(obs->time);
            if (fabs(xt - tt[obs->sat - 1]) / xs > 0.9) {
                graphR->drawMark(c, xt, yp[obs->sat - 1], 0, plotStyle < 2 ? col : cColor[3],
                         plotStyle < 2 ? markSize : 0, 0);
                tt[obs->sat - 1] = xt;
            }
        }
    }
    if (level && plotStyle <= 2)
        drawObservationSlip(c, yp);

    if (btnShowTrack->isChecked() && 0 <= ind && ind < nObservation) {
        i = indexObservation[ind];
        time = observation.data[i].time;

        graphR->getLimits(xl, yl);
        xl[0] = xl[1] = timePosition(observation.data[i].time);
        graphR->drawPoly(c, xl, yl, 2, cColor[2], 0);

        for (; i < observation.n && timediff(observation.data[i].time, time) == 0.0; i++) {
            obs = &observation.data[i];
            QColor col = observationColor(obs, azimuth[i], elevtion[i]);
            if (col == Qt::black) continue;
            graphR->drawMark(c, xl[0], yp[obs->sat - 1], 0, col, markSize * 2 + 2, 0);
        }
        graphR->drawMark(c, xl[0], yl[1] - 1E-6, 0, cColor[2], 5, 0);
        if (!btnFixHorizontal->isChecked())
            graphR->drawMark(c, xl[0], yl[1] - 1E-6, 1, cColor[2], 9, 0);
    }
}
// generate observation-data-slips ---------------------------------------
void Plot::generateObservationSlip(int *LLI)
{
    QString obstype = cBObservationType->currentText();
    bool ok;
    double gfp[MAXSAT][NFREQ+NEXOBS]={{0}};

    for (int i=0;i<observation.n;i++) {
        obsd_t *obs=observation.data+i;
        int j,k;

        LLI[i]=0;
        if (elevtion[i]<elevationMask*D2R||!satelliteSelection[obs->sat-1]) continue;
        if (elevationMaskP&&elevtion[i]<elevationMaskData[(int)(azimuth[i]*R2D+0.5)]) continue;

        if (showSlip==1) { // LG jump
            double freq1,freq2,gf;

            if (obstype == "ALL") {
                if ((freq1=sat2freq(obs->sat,obs->code[0],&navigation))==0.0) continue;
                LLI[i]=obs->LLI[0]&2;
                for (j=1;j<NFREQ+NEXOBS;j++) {
                    LLI[i]|=obs->LLI[j]&2;
                    if ((freq2=sat2freq(obs->sat,obs->code[j],&navigation))==0.0) continue;
                    gf=CLIGHT*(obs->L[0]/freq1-obs->L[j]/freq2);
                    if (fabs(gfp[obs->sat-1][j]-gf)>THRESLIP) LLI[i]|=1;
                    gfp[obs->sat-1][j]=gf;
                }
            }
            else {
                k = obstype.mid(1).toInt(&ok);
                if (ok) {
                    j=k-1;
                }
                else {
                    for (j=0;j<NFREQ+NEXOBS;j++) {
                        if (!strcmp(code2obs(obs->code[j]),qPrintable(obstype))) break;
                    }
                    if (j>=NFREQ+NEXOBS) continue;
                }
                LLI[i]=obs->LLI[j]&2;
                k=(j==0)?1:0;
                if ((freq1=sat2freq(obs->sat,obs->code[k],&navigation))==0.0) continue;
                if ((freq2=sat2freq(obs->sat,obs->code[j],&navigation))==0.0) continue;
                gf=CLIGHT*(obs->L[k]/freq1-obs->L[j]/freq2);
                if (fabs(gfp[obs->sat-1][j]-gf)>THRESLIP) LLI[i]|=1;
                gfp[obs->sat-1][j]=gf;
            }
        }
        else {
            if (obstype == "ALL") {
                for (j=0;j<NFREQ+NEXOBS;j++) {
                    LLI[i]|=obs->LLI[j];
                }
            }
            else {
                k = obstype.mid(1).toInt(&ok);
                if (ok) {
                    j=k-1;
                }
                else {
                    for (j=0;j<NFREQ+NEXOBS;j++) {
                        if (!strcmp(code2obs(obs->code[j]),qPrintable(obstype))) break;
                    }
                    if (j>=NFREQ+NEXOBS) continue;
                }
                LLI[i]=obs->LLI[j];
            }
        }
    }
}
// draw slip on observation-data-plot ---------------------------------------
void Plot::drawObservationSlip(QPainter &c, double *yp)
{
    trace(3,"DrawObsSlip\n");
    if (observation.n<=0||(!showSlip&&!showHalfC)) return;

    int *LLI=new int [observation.n];

    generateObservationSlip(LLI);

    for (int i=0;i<observation.n;i++) {
        if (!LLI[i]) continue;
        QPoint ps[2];
        obsd_t *obs=observation.data+i;
        if (graphR->toPoint(timePosition(obs->time),yp[obs->sat-1],ps[0])) {
            ps[1].setX(ps[0].x());
            ps[1].setY(ps[0].y()+markSize*3/2+1);
            ps[0].setY(ps[0].y()-markSize*3/2);
            if (showHalfC&&(LLI[i]&2)) graphR->drawPoly(c, ps, 2, mColor[0][0], 0);
            if (showSlip &&(LLI[i]&1)) graphR->drawPoly(c, ps, 2, mColor[0][5], 0);
        }
    }
    delete [] LLI;
}
// draw ephemeris on observation-data-plot ----------------------------------
void Plot::drawObservationEphemeris(QPainter &c, double *yp)
{
    QPoint ps[3];
    int i, j, k, in, svh, off[MAXSAT] = { 0 };

    trace(3, "DrawObsEphem\n");

    if (!showEphemeris) return;

    for (i = 0; i < MAXSAT; i++) {
        if (!satelliteSelection[i]) continue;
        for (j = 0; j < navigation.n; j++) {
            if (navigation.eph[j].sat != i + 1) continue;
            graphR->toPoint(timePosition(navigation.eph[j].ttr), yp[i], ps[0]);
            in = graphR->toPoint(timePosition(navigation.eph[j].toe), yp[i], ps[2]);
            ps[1] = ps[0];
            off[navigation.eph[j].sat - 1] = off[navigation.eph[j].sat - 1] ? 0 : 3;

            for (k = 0; k < 3; k++) ps[k].ry() += markSize + 2 + off[navigation.eph[j].sat - 1];
            ps[0].ry() -= 2;

            svh = navigation.eph[j].svh;
            if (satsys(i + 1, NULL) == SYS_QZS) svh &= 0xFE; /* mask QZS LEX health */

            graphR->drawPoly(c, ps, 3, svh ? mColor[0][5] : cColor[1], 0);

            if (in) graphR->drawMark(c, ps[2], 0, svh ? mColor[0][5] : cColor[1], svh ? 4 : 3, 0);
        }
        for (j = 0; j < navigation.ng; j++) {
            if (navigation.geph[j].sat != i + 1) continue;
            graphR->toPoint(timePosition(navigation.geph[j].tof), yp[i], ps[0]);
            in = graphR->toPoint(timePosition(navigation.geph[j].toe), yp[i], ps[2]);
            ps[1] = ps[0];
            off[navigation.geph[j].sat - 1] = off[navigation.geph[j].sat - 1] ? 0 : 3;
            for (k = 0; k < 3; k++) ps[k].ry() += markSize + 2 + off[navigation.geph[j].sat - 1];
            ps[0].ry() -= 2;

            graphR->drawPoly(c, ps, 3, navigation.geph[j].svh ? mColor[0][5] : cColor[1], 0);

            if (in) graphR->drawMark(c, ps[2], 0, navigation.geph[j].svh ? mColor[0][5] : cColor[1],
                         navigation.geph[j].svh ? 4 : 3, 0);
        }
        for (j = 0; j < navigation.ns; j++) {
            if (navigation.seph[j].sat != i + 1) continue;
            graphR->toPoint(timePosition(navigation.seph[j].tof), yp[i], ps[0]);
            in = graphR->toPoint(timePosition(navigation.seph[j].t0), yp[i], ps[2]);
            ps[1] = ps[0];
            off[navigation.seph[j].sat - 1] = off[navigation.seph[j].sat - 1] ? 0 : 3;
            for (k = 0; k < 3; k++) ps[k].ry() += markSize + 2 + off[navigation.seph[j].sat - 1];
            ps[0].ry() -= 2;

            graphR->drawPoly(c, ps, 3, navigation.seph[j].svh ? mColor[0][5] : cColor[1], 0);

            if (in) graphR->drawMark(c, ps[2], 0, navigation.seph[j].svh ? mColor[0][5] : cColor[1],
                         navigation.seph[j].svh ? 4 : 3, 0);
        }
    }
}
// draw sky-image on sky-plot -----------------------------------------------
void Plot::drawSkyImage(QPainter &c, int level)
{
    QPoint p1, p2;
    double xl[2], yl[2], r, s, mx[190], my[190];

    trace(3, "DrawSkyImage: level=%d\n", level);

    if (skySize[0] <= 0 || skySize[1] <= 0) return;

    graphS->getLimits(xl, yl);
    r = (xl[1] - xl[0] < yl[1] - yl[0] ? xl[1] - xl[0] : yl[1] - yl[0]) * 0.45;
    s = r * skyImageR.width() / 2.0 / skyScaleR;
    graphS->toPoint(-s, s, p1);
    graphS->toPoint(s, -s, p2);
    QRect rect(p1, p2);
    c.drawImage(rect, skyImageR);

    if (skyElevationMask) { // elevation mask
        int n = 0;

        mx[n] = 0.0;   my[n++] = yl[1];
        for (int i = 0; i <= 180; i++) {
            mx[n] = r * sin(i * 2.0 * D2R);
            my[n++] = r * cos(i * 2.0 * D2R);
        }
        mx[n] = 0.0;   my[n++] = yl[1];
        mx[n] = xl[0]; my[n++] = yl[1];
        mx[n] = xl[0]; my[n++] = yl[0];
        mx[n] = xl[1]; my[n++] = yl[0];
        mx[n] = xl[1]; my[n++] = yl[1];
        graphS->drawPatch(c, mx, my, n, cColor[0], cColor[0], 0);
    }
}
// draw sky-plot ------------------------------------------------------------
void Plot::drawSky(QPainter &c, int level)
{
    QPoint p1, p2;
    QString s, obstype = cBObservationType->currentText();
    obsd_t *obs;
    gtime_t t[MAXSAT] = { { 0, 0 } };
    double p[MAXSAT][2] = { { 0 } }, p0[MAXSAT][2] = { { 0 } };
    double x, y, xp, yp, xs, ys, dt, dx, dy, xl[2], yl[2], r;
    int i, j, ind = observationIndex, freq;
    int hh = static_cast<int>(lblDisplay->font().pointSize() * 1.5);

    trace(3, "DrawSky: level=%d\n", level);

    graphS->getLimits(xl, yl);
    r = (xl[1] - xl[0] < yl[1] - yl[0] ? xl[1] - xl[0] : yl[1] - yl[0]) * 0.45;

    if (btnShowImage->isChecked())
        drawSkyImage(c, level);
    if (btnShowSkyplot->isChecked())
        graphS->drawSkyPlot(c, 0.0, 0.0, cColor[1], cColor[2], cColor[0], r * 2.0);
    if (!btnSolution1->isChecked()) return;

    graphS->getScale(xs, ys);

    if (plotStyle <= 2) {
        for (i = 0; i < observation.n; i++) {
            obs = &observation.data[i];
            if (satelliteMask[obs->sat - 1] || !satelliteSelection[obs->sat - 1] || elevtion[i] <= 0.0) continue;
            QColor col = observationColor(obs, azimuth[i], elevtion[i]);
            if (col == Qt::black) continue;

            x = r * sin(azimuth[i]) * (1.0 - 2.0 * elevtion[i] / PI);
            y = r * cos(azimuth[i]) * (1.0 - 2.0 * elevtion[i] / PI);
            xp = p[obs->sat - 1][0];
            yp = p[obs->sat - 1][1];

            if ((x - xp) * (x - xp) + (y - yp) * (y - yp) >= xs * xs) {
                int siz = plotStyle < 2 ? markSize : 1;
                graphS->drawMark(c, x, y, 0, plotStyle < 2 ? col : cColor[3], siz, 0);
                p[obs->sat - 1][0] = x;
                p[obs->sat - 1][1] = y;
            }
            if (xp == 0.0 && yp == 0.0) {
                p0[obs->sat - 1][0] = x;
                p0[obs->sat - 1][1] = y;
            }
        }
    }
    if ((plotStyle == 0 || plotStyle == 2) && !btnShowTrack->isChecked()) {
        for (i = 0; i < MAXSAT; i++) {
            if (p0[i][0] != 0.0 || p0[i][1] != 0.0) {
                QPoint pnt;
                if (graphS->toPoint(p0[i][0], p0[i][1], pnt)) {
                    char id[16];
                    satno2id(i+1,id);
                    drawLabel(graphS, c, pnt, QString(id), 1, 0);
                }
            }
        }
    }
    if (!level) return;

    if (showSlip && plotStyle <= 2) {
        int *LLI=new int [observation.n];

        generateObservationSlip(LLI);

        for (i = 0; i < observation.n; i++) {
            if (!(LLI[i]&1)) continue;
            obs=observation.data+i;
            x = r * sin(azimuth[i]) * (1.0 - 2.0 * elevtion[i] / PI);
            y = r * cos(azimuth[i]) * (1.0 - 2.0 * elevtion[i] / PI);
            dt = timediff(obs->time, t[obs->sat - 1]);
            dx = x - p[obs->sat - 1][0];
            dy = y - p[obs->sat - 1][1];
            t[obs->sat - 1] = obs->time;
            p[obs->sat - 1][0] = x;
            p[obs->sat - 1][1] = y;
            if (fabs(dt) > 300.0) continue;
            graphS->drawMark(c, x, y, 4, mColor[0][5], markSize * 3 + 2, static_cast<int>(ATAN2(dy, dx) * R2D + 90));
        }
        delete [] LLI;
    }

    if (elevationMaskP) {
        double *x = new double [361];
        double *y = new double [361];
        for (i = 0; i <= 360; i++) {
            x[i] = r * sin(i * D2R) * (1.0 - 2.0 * elevationMaskData[i] / PI);
            y[i] = r * cos(i * D2R) * (1.0 - 2.0 * elevationMaskData[i] / PI);
        }
        QPen pen = c.pen(); pen.setWidth(2); c.setPen(pen);
        graphS->drawPoly(c, x, y, 361, COL_ELMASK, 0);
        pen.setWidth(1); c.setPen(pen);
        delete [] x;
        delete [] y;
    }

    if (btnShowTrack->isChecked() && 0 <= ind && ind < nObservation) {
        for (i = indexObservation[ind]; i < observation.n && i < indexObservation[ind + 1]; i++) {
            obs = &observation.data[i];
            if (satelliteMask[obs->sat - 1] || !satelliteSelection[obs->sat - 1] || elevtion[i] <= 0.0) continue;
            QColor col = observationColor(obs, azimuth[i], elevtion[i]);
            if (col == Qt::black) continue;

            x = r * sin(azimuth[i]) * (1.0 - 2.0 * elevtion[i] / PI);
            y = r * cos(azimuth[i]) * (1.0 - 2.0 * elevtion[i] / PI);

            char id[16];
            satno2id(obs->sat, id);
            graphS->drawMark(c, x, y, 0, col, lblDisplay->font().pointSize() * 2 + 5, 0);
            graphS->drawMark(c, x, y, 1, col == Qt::black ? mColor[0][0] : cColor[2], lblDisplay->font().pointSize() * 2 + 5, 0);
            graphS->drawText(c, x, y, QString(id), cColor[0], 0, 0, 0);
        }
    }

    graphS->getPosition(p1, p2);
    p1.rx() += 10; p1.ry() += 8; p2.rx() -= 10; p2.ry() = p1.y();

    if (showStats && !simObservation) {
        s = QString(tr("MARKER: %1 %2")).arg(station.name).arg(station.marker);
        drawLabel(graphS, c, p1, s, 1, 2); p1.ry() += hh;
        s = QString(tr("REC: %1 %2 %3")).arg(station.rectype).arg(station.recver).arg(station.recsno);
        drawLabel(graphS, c, p1, s, 1, 2); p1.ry() += hh;
        s = QString(tr("ANT: %1 %2")).arg(station.antdes).arg(station.antsno);
        drawLabel(graphS, c, p1, s, 1, 2); p1.ry() += hh;
    }
        // show statistics
    if (showStats && btnShowTrack->isChecked() && 0 <= ind && ind < nObservation && !simObservation) {
        char id[16];

        if (obstype == "ALL") {
            s = QString::asprintf("%3s: %*s %*s%*s %*s","SAT",NFREQ,"PR",NFREQ,"CP",
                         NFREQ*3,"CN0",NFREQ,"LLI");
        }
        else {
            s = QString(tr("SAT: SIG OBS CN0 LLI"));
        }
        graphS->drawText(c, p2, s, Qt::black, 2, 2, 0, QFont(MS_FONT));

        p2.ry() += 3;

        for (i = indexObservation[ind]; i < observation.n && i < indexObservation[ind + 1]; i++) {
            bool ok;
            obs = &observation.data[i];
            if (satelliteMask[obs->sat - 1] || !satelliteSelection[obs->sat - 1]) continue;
            if (hideLowSatellites && elevtion[i] < elevationMask * D2R) continue;
            if (hideLowSatellites && elevationMaskP && elevtion[i] < elevationMaskData[static_cast<int>(azimuth[i] * R2D + 0.5)]) continue;

            satno2id(obs->sat, id);
            s = QString("%1: ").arg(id, 3, QChar('-'));

            freq=obstype.mid(1).toInt(&ok);

            if (obstype == "ALL") {
                for (j = 0; j < NFREQ; j++) s += obs->P[j] == 0.0 ? "-" : "C";
                s += " ";
                for (j = 0; j < NFREQ; j++) s += obs->L[j] == 0.0 ? "-" : "L";
                s += " ";
                for (j = 0; j < NFREQ; j++) {
                    if (obs->P[j]==0.0&&obs->L[j]==0.0) s+="-- ";
                    else s += QString("%1 ").arg(obs->SNR[j] * SNR_UNIT, 2, 'f', 0, QChar('0'));
                }
                for (j = 0; j < NFREQ; j++) {
                    if (obs->L[j]==0.0) s+="-";
                    else s += QString("%1").arg(obs->LLI[j]);
                }
             } else if (ok) {
                if (!obs->code[freq-1]) continue;
                s+=QString("%1  %2 %3 %4  ").arg(code2obs(obs->code[freq-1])).arg(
                              obs->P[freq-1]==0.0?"-":"C").arg(obs->L[freq-1]==0.0?"-":"L").arg(
                              obs->D[freq-1]==0.0?"-":"D");
                if (obs->P[freq-1]==0.0&&obs->L[freq-1]==0.0) s+="---- ";
                else s+=QString("%1 ").arg(obs->SNR[freq-1]*SNR_UNIT,4,'f',1);
                if (obs->L[freq-1]==0.0) s+=" -";
                else s+=QString::number(obs->LLI[freq-1]);
            } else {
                for (j = 0; j < NFREQ + NEXOBS; j++)
                    if (!strcmp(code2obs(obs->code[j]), qPrintable(obstype))) break;
                if (j >= NFREQ + NEXOBS) continue;

                s+=QString("%1  %2 %3 %4  ").arg(code2obs(obs->code[j])).arg(
                                 obs->P[j]==0.0?"-":"C").arg(obs->L[j]==0.0?"-":"L").arg(
                                 obs->D[j]==0.0?"-":"D");
                if (obs->P[j]==0.0&&obs->L[j]==0.0) s+="---- ";
                else s+=QString("%1 ").arg(obs->SNR[j]*SNR_UNIT,4,'f',1);
                if (obs->L[j]==0.0) s+=" -";
                else s+=QString::number(obs->LLI[j]);
            }
            QColor col = observationColor(obs, azimuth[i], elevtion[i]);
            p2.ry() += hh;
            graphS->drawText(c, p2, s, col == Qt::black ? mColor[0][0] : col, 2, 2, 0);
        }
    }
    if (navigation.n <= 0 && navigation.ng <= 0 && !simObservation) {
        graphS->getPosition(p1, p2);
        p2.rx() -= 10;
        p2.ry() -= 3;
        drawLabel(graphS, c, p2, tr("No Navigation Data"), 2, 1);
    }
}
// draw DOP and number-of-satellite plot ------------------------------------
void Plot::drawDop(QPainter &c, int level)
{
    QString label;
    QPoint p1, p2;
    double xp, xc, yc, xl[2], yl[2], azel[MAXSAT * 2], *dop, *x, *y;
    int i, j, *ns, n = 0;
    int ind = observationIndex, doptype = cBDopType->currentIndex();

    trace(3, "DrawDop: level=%d\n", level);
    
    graphR->xLabelPosition = timeLabel ? 6 : 1;
    graphR->yLabelPosition = 1;
    graphR->week = week;
    graphR->getLimits(xl, yl);
    yl[0] = 0.0;
    yl[1] = maxDop;
    graphR->setLimits(xl, yl);
    graphR->setTick(0.0, 0.0);

    if (0 <= ind && ind < nObservation && btnShowTrack->isChecked() && btnFixHorizontal->isChecked()) {
        double xl[2], yl[2], off;
        graphR->getLimits(xl, yl);
        off = Xcent * (xl[1] - xl[0]) / 2.0;
        xp = timePosition(observation.data[indexObservation[ind]].time);
        graphR->getCenter(xc, yc);
        graphR->setCenter(xp - off, yc);
    }
    graphR->drawAxis(c, 1, 1);
    graphR->getPosition(p1, p2);
    p1.setX(lblDisplay->font().pointSize());
    p1.setY((p1.y() + p2.y()) / 2);
    if (doptype == 0)
        label = QString("# OF SATELLITES / DOP (EL>=%1%2)").arg(elevationMask, 0, 'f', 0).arg(degreeChar);
    else if (doptype == 1)
        label = QString("# OF SATELLITES (EL>=%1%2)").arg(elevationMask, 0, 'f', 0).arg(degreeChar);
    else
        label = QString("DOP (EL>=%1%2)").arg(elevationMask, 0, 'f', 0).arg(degreeChar);
    graphR->drawText(c, p1, label, cColor[2], 0, 0, 90);

    if (!btnSolution1->isChecked()) return;

    x = new double[nObservation];
    y = new double[nObservation];
    dop = new double[nObservation * 4];
    ns = new int   [nObservation];

    for (i = 0; i < nObservation; i++) {
        ns[n] = 0;
        for (j = indexObservation[i]; j < observation.n && j < indexObservation[i + 1]; j++) {
            if (satelliteMask[observation.data[j].sat - 1] || !satelliteSelection[observation.data[j].sat - 1]) continue;
            if (elevtion[j] < elevationMask * D2R) continue;
            if (elevationMaskP && elevtion[j] < elevationMaskData[static_cast<int>(azimuth[j] * R2D + 0.5)]) continue;
            azel[ns[n] * 2] = azimuth[j];
            azel[1 + ns[n] * 2] = elevtion[j];
            ns[n]++;
        }
        dops(ns[n], azel, elevationMask * D2R, dop + n * 4);
        x[n++] = timePosition(observation.data[indexObservation[i]].time);
    }
    for (i = 0; i < 4; i++) {
        if (doptype != 0 && doptype != i + 2) continue;

        for (j = 0; j < n; j++) y[j] = dop[i + j * 4]*10;

        if (!(plotStyle % 2))
            drawPolyS(graphR, c, x, y, n, cColor[3], 0);
        if (level && plotStyle < 2) {
            for (j = 0; j < n; j++) {
                if (y[j] == 0.0) continue;
                graphR->drawMark(c, x[j], y[j], 0, mColor[0][i + 2], markSize, 0);
            }
        }
    }
    if (doptype == 0 || doptype == 1) {
        for (i = 0; i < n; i++) y[i] = ns[i];

        if (!(plotStyle % 2))
            drawPolyS(graphR, c, x, y, n, cColor[3], 1);
        if (level && plotStyle < 2) {
            for (i = 0; i < n; i++)
                graphR->drawMark(c, x[i], y[i], 0, mColor[0][1], markSize, 0);
        }
    }

    if (btnShowTrack->isChecked() && 0 <= ind && ind < nObservation) {
        graphR->getLimits(xl, yl);
        xl[0] = xl[1] = timePosition(observation.data[indexObservation[ind]].time);

        graphR->drawPoly(c, xl, yl, 2, cColor[2], 0);

        ns[0] = 0;
        for (i = indexObservation[ind]; i < observation.n && i < indexObservation[ind + 1]; i++) {
            if (satelliteMask[observation.data[i].sat - 1] || !satelliteSelection[observation.data[i].sat - 1]) continue;
            if (elevtion[i] < elevationMask * D2R) continue;
            if (elevationMaskP && elevtion[i] < elevationMaskData[static_cast<int>(azimuth[i] * R2D + 0.5)]) continue;
            azel[ns[0] * 2] = azimuth[i];
            azel[1 + ns[0] * 2] = elevtion[i];
            ns[0]++;
        }
        dops(ns[0], azel, elevationMask * D2R, dop);

        for (i = 0; i < 4; i++) {
            if ((doptype != 0 && doptype != i + 2) || dop[i] <= 0.0) continue;
            graphR->drawMark(c, xl[0], dop[i]*10.0, 0, mColor[0][i + 2], markSize * 2 + 2, 0);
        }
        if (doptype == 0 || doptype == 1)
            graphR->drawMark(c, xl[0], ns[0], 0, mColor[0][1], markSize * 2 + 2, 0);
        graphR->drawMark(c, xl[0], yl[1] - 1E-6, 0, cColor[2], 5, 0);
        if (!btnFixHorizontal->isChecked())
            graphR->drawMark(c, xl[0], yl[1] - 1E-6, 1, cColor[2], 9, 0);
    } else {
        drawDopStat(c, dop, ns, n);
    }

    if (navigation.n <= 0 && navigation.ng <= 0 && (doptype == 0 || doptype >= 2) && !simObservation) {
        graphR->getPosition(p1, p2);
        p2.rx() -= 10;
        p2.ry() -= 3;
        drawLabel(graphR, c, p2, tr("No Navigation Data"), 2, 1);
    }

    delete [] x;
    delete [] y;
    delete [] dop;
    delete [] ns;
}
// draw statistics on DOP and number-of-satellite plot ----------------------
void Plot::drawDopStat(QPainter &c, double *dop, int *ns, int n)
{
    QString s0[MAXOBS + 2], s1[MAXOBS + 2], s2[MAXOBS + 2];
    double ave[4] = { 0 };
    int nsat[MAXOBS]={0},ndop[4]={0}, m = 0;

    trace(3, "DrawDopStat: n=%d\n", n);

    if (!showStats) return;

    for (int i = 0; i < n; i++) {
        nsat[ns[i]]++;
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < n; j++) {
            if (dop[i + j * 4] <= 0.0 || dop[i + j * 4] > maxDop) continue;
            ave[i] += dop[i + j * 4];
            ndop[i]++;
        }
        if (ndop[i] > 0) ave[i] /= ndop[i];
    }
    if (cBDopType->currentIndex() == 0 || cBDopType->currentIndex() >= 2) {
        s2[m++] = QString("AVE= GDOP:%1 PDOP:%2 HDOP:%3 VDOP:%4")
              .arg(ave[0], 4, 'f', 1).arg(ave[1], 4, 'f', 1).arg(ave[2], 4, 'f', 1).arg(ave[3], 4, 'f', 1);
        s2[m++] = QString("NDOP=%1(%2%%) %3(%4%%) %5(%6%%) %7(%8f%%)")
              .arg(ndop[0]).arg(n > 0 ? ndop[0] * 100.0 / n : 0.0, 4, 'f', 1)
              .arg(ndop[1]).arg(n > 0 ? ndop[1] * 100.0 / n : 0.0, 4, 'f', 1)
              .arg(ndop[2]).arg(n > 0 ? ndop[2] * 100.0 / n : 0.0, 4, 'f', 1)
              .arg(ndop[3]).arg(n > 0 ? ndop[3] * 100.0 / n : 0.0, 4, 'f', 1);
    }
    if (cBDopType->currentIndex() <= 1) {
        for (int i = 0, j = 0; i < MAXOBS; i++) {
            if (nsat[i] <= 0) continue;
            s0[m] = QString("%1%2:").arg(j++ == 0 ? "NSAT= " : "").arg(i, 2);
            s1[m] = QString("%1").arg(nsat[i], 7);
            s2[m++] = QString("(%1%%)").arg(nsat[i] * 100.0 / n, 4, 'f', 1);
        }
    }
    QPoint p1,p2,p3;
    int fonth=(int)(lblDisplay->font().pixelSize()*1.5);

    graphR->getPosition(p1, p2);
    p1.setX(p2.x() - 10);
    p1.ry() += 8;
    p2 = p1; p2.rx() -= fonth * 4;
    p3 = p2; p3.rx() -= fonth * 8;

    for (int i = 0; i < m; i++) {
        drawLabel(graphR, c, p3, s0[i], 2, 2);
        drawLabel(graphR, c, p2, s1[i], 2, 2);
        drawLabel(graphR, c, p1, s2[i], 2, 2);
        p1.setY(p1.y() + fonth);
        p2.setY(p2.y() + fonth);
        p3.setY(p3.y() + fonth);
    }
}
// draw SNR, MP and elevation-plot ---------------------------------------------
void Plot::drawSnr(QPainter &c, int level)
{
    QPushButton *btn[] = { btnOn1, btnOn2, btnOn3 };
    QString ObsTypeText = cBObservationType2->currentText();
    QString label[] = { tr("SNR"), tr("Multipath"), tr("Elevation") };
    QString unit[] = { "dBHz", "m", degreeChar };
    gtime_t time = { 0, 0 };

    trace(3, "DrawSnr: level=%d\n", level);

    if (0 <= observationIndex && observationIndex < nObservation && btnShowTrack->isChecked())
        time = observation.data[indexObservation[observationIndex]].time;
    if (0 <= observationIndex && observationIndex < nObservation && btnShowTrack->isChecked() && btnFixHorizontal->isChecked()) {
        double xc,yc,xp=timePosition(time);
        double xl[2],yl[2];

        graphG[0]->getLimits(xl, yl);
        xp -= Xcent * (xl[1] - xl[0]) / 2.0;
        for (int i=0;i<3;i++) {
            graphG[i]->getCenter(xc,yc);
            graphG[i]->setCenter(xp,yc);
        }
    }
    int j = 0;

    for (int i = 0; i < 3; i++) if (btn[i]->isChecked()) j = i;

    for (int i = 0; i < 3; i++) {
        if (!btn[i]->isChecked()) continue;
        graphG[i]->xLabelPosition = timeLabel ? (i == j ? 6 : 5) : (i == j ? 1 : 0);
        graphG[i]->week = week;
        graphG[i]->drawAxis(c, showLabel, showLabel);
    }

    if (nObservation > 0 && btnSolution1->isChecked()) {
        QString obstype=cBObservationType2->currentText();
        double *x=new double[nObservation];
        double *y=new double[nObservation];
        QColor *col=new QColor[nObservation];

        for (int i=0,l=0;i<3;i++) {
            QColor colp[MAXSAT];
            double yp[MAXSAT],ave=0.0,rms=0.0;
            int np=0,nrms=0;

            if (!btn[i]->isChecked()) continue;

            for (int sat = 1, np = 0; sat <= MAXSAT; sat++) {
                if (satelliteMask[sat - 1] || !satelliteSelection[sat - 1]) continue;
                int n=0;

                for (int j = n = 0; j < observation.n; j++) {
                    obsd_t *obs=observation.data+j;
                    int k,freq;
                    bool ok;

                    if (obs->sat!=sat) continue;

                    freq=obstype.mid(1).toInt(&ok);
                    if (ok) {
                        k=freq-1;
                    }
                    else {
                        for (k=0;k<NFREQ+NEXOBS;k++) {
                            if (!strcmp(code2obs(obs->code[k]),qPrintable(obstype))) break;
                        }
                        if (k>=NFREQ+NEXOBS) continue;
                    }
                    if (obs->SNR[k]*SNR_UNIT<=0.0) continue;

                    x[n] = timePosition(obs->time);
                    if (i == 0) {
                        y[n] = obs->SNR[k] * SNR_UNIT;
                        col[n] = mColor[0][4];
                    } else if (i == 1) {
                        if (!multipath[k] || multipath[k][j] == 0.0) continue;
                        y[n] = multipath[k][j];
                        col[n] = mColor[0][4];
                    } else {
                        y[n] = elevtion[j] * R2D;
                        if (simObservation) col[n] = sysColor(obs->sat);
                        else col[n] = snrColor(obs->SNR[k] * SNR_UNIT);
                        if (elevtion[j] > 0.0 && elevtion[j] < elevationMask * D2R) col[n] = mColor[0][0];
                    }
                    if (timediff(time, obs->time) == 0.0 && np < MAXSAT) {
                        yp[np] = y[n];
                        colp[np++] = col[n];
                    }
                    if (n < nObservation) n++;
                }
                if (!level || !(plotStyle % 2)) {
                    for (int j = 0, k=0; j < n; j = k) {
                        for (k = j + 1; k < n; k++) if (fabs(y[k - 1] - y[k]) > 30.0) break;
                        drawPolyS(graphG[i], c, x + j, y + j, k - j, cColor[3], 0);
                    }
                }
                if (level && plotStyle < 2) {
                    for (int j  = 0; j < n; j++) {
                        if (i != 1 && y[j] <= 0.0) continue;
                        graphG[i]->drawMark(c, x[j], y[j], 0, col[j], markSize, 0);
                    }
                }
                for (int j = 0; j < n; j++) {
                    if (y[j] == 0.0) continue;
                    ave += y[j];
                    rms += SQR(y[j]);
                    nrms++;
                }
            }
            if (level && i == 1 && nrms > 0 && showStats && !btnShowTrack->isChecked()) {
                QPoint p1,p2;
                ave=ave/nrms;
                rms=SQRT(rms/nrms);

                graphG[i]->getPosition(p1, p2);
                p1.rx() = p2.x() - 8; p1.ry() += 3;
                drawLabel(graphG[i], c, p1, QString("AVE=%1m RMS=%2m").arg(ave, 0, 'f', 4)
                      .arg(rms, 0, 'f', 4), 2, 2);
            }
            if (btnShowTrack->isChecked() && 0 <= observationIndex && observationIndex < nObservation && btnSolution1->isChecked()) {
                if (!btn[i]->isChecked()) continue;
                QPoint p1,p2;
                double xl[2],yl[2];
                graphG[i]->getLimits(xl, yl);
                xl[0] = xl[1] = timePosition(time);
                graphG[i]->drawPoly(c, xl, yl, 2, cColor[2], 0);

                if (l++ == 0) {
                    graphG[i]->drawMark(c, xl[0], yl[1] - 1E-6, 0, cColor[2], 5, 0);

                    if (!btnFixHorizontal->isChecked())
                        graphG[i]->drawMark(c, xl[0], yl[1] - 1E-6, 1, cColor[2], 9, 0);
                }
                for (int k = 0; k < np; k++) {
                    if (i != 1 && yp[k] <= 0.0) continue;
                    graphG[i]->drawMark(c, xl[0], yp[k], 0, cColor[0], markSize * 2 + 4, 0);
                    graphG[i]->drawMark(c, xl[0], yp[k], 0, colp[k], markSize * 2 + 2, 0);
                }
                if (np <= 0 || np > 1 || (i != 1 && yp[0] <= 0.0)) continue;

                graphG[i]->getPosition(p1, p2);
                p1.rx() = p2.x() - 8; p1.ry() += 3;
                drawLabel(graphG[i], c, p1, QString("%1 %2").arg(yp[0], 0, 'f', i == 1 ? 4 : 1).arg(unit[i]), 2, 2);
            }
        }
        delete [] x;
        delete [] y;
        delete [] col;
    }
    for (int i = 0; i < 3; i++) {
        if (!btn[i]->isChecked()) continue;
        QPoint p1, p2;
        graphG[i]->getPosition(p1, p2);
        p1.rx() += 5; p1.ry() += 3;
        drawLabel(graphG[i], c, p1, QString("%1 (%2)").arg(label[i]).arg(unit[i]), 1, 2);
    }
}
// draw SNR, MP to elevation-plot ----------------------------------------------
void Plot::drawSnrE(QPainter &c, int level)
{
    QPushButton *btn[] = { btnOn1, btnOn2, btnOn3 };
    QString s, ObsTypeText = cBObservationType2->currentText();
    QString label[] = { tr("SNR (dBHz)"), tr("Multipath (m)") };
    gtime_t time = { 0, 0 };
    double ave=0.0, rms=0.0;

    int nrms=0;

    trace(3, "DrawSnrE: level=%d\n", level);

    int j=0;
    for (int i=0;i<2;i++) if (btn[i]->isChecked()) j=i;

    for (int i=0;i<2;i++) {
        QPoint p1,p2;
        double xl[2]={-0.001,90.0},yl[2][2]={{10.0,65.0},{-maxMP,maxMP}};

        if (!btn[i]->isChecked()) continue;
        graphE[i]->xLabelPosition = i == j ? 1 : 0;
        graphE[i]->yLabelPosition = 1;
        graphE[i]->setLimits(xl, yl[i]);
        graphE[i]->setTick(0.0, 0.0);
        graphE[i]->drawAxis(c, 1, 1);

        graphE[i]->getPosition(p1, p2);
        p1.setX(lblDisplay->font().pointSize());
        p1.setY((p1.y() + p2.y()) / 2);
        graphE[i]->drawText(c, p1, label[i], cColor[2], 0, 0, 90);
        if (i == j) {
            p2.rx() -= 8; p2.ry() -= 6;
            graphE[i]->drawText(c, p2, tr("Elevation ( %1 )").arg(degreeChar), cColor[2], 2, 1, 0);
        }
    }

    if (0 <= observationIndex && observationIndex < nObservation && btnShowTrack->isChecked())
        time = observation.data[indexObservation[observationIndex]].time;

    if (nObservation > 0 && btnSolution1->isChecked()) {
        QColor *col[2],colp[2][MAXSAT];
        QString obstype=cBObservationType2->currentText();
        double *x[2],*y[2],xp[2][MAXSAT],yp[2][MAXSAT];
        int n[2],np[2]={0};

        for (int i = 0; i < 2; i++) {
            x[i] = new double[nObservation],
            y[i] = new double[nObservation];
            col[i] = new QColor[nObservation];
        }
        for (int sat = 1; sat <= MAXSAT; sat++) {
            if (satelliteMask[sat - 1] || !satelliteSelection[sat - 1]) continue;
            n[0]=n[1]=0;

            for (int j=0;j<observation.n;j++) {
                obsd_t *obs=observation.data+j;
                int k,freq;
                bool ok;

                if (obs->sat!=sat||elevtion[j]<=0.0) continue;

                freq = obstype.mid(1).toInt(&ok);

                if (ok) {
                    k=freq-1;
                }
                else {
                    for (k=0;k<NFREQ+NEXOBS;k++) {
                        if (!strcmp(code2obs(obs->code[k]),qPrintable(obstype))) break;
                    }
                    if (k>=NFREQ+NEXOBS) continue;
                }
                if (obs->SNR[k]*SNR_UNIT<=0.0) continue;

                y[0][n[0]]=obs->SNR[k]*SNR_UNIT;
                y[1][n[1]] = !multipath[k] ? 0.0 : multipath[k][j];

                col[0][n[0]] = col[1][n[1]] =
                               elevtion[j] > 0.0 && elevtion[j] < elevationMask * D2R ? mColor[0][0] : mColor[0][4];

                if (y[0][n[0]] > 0.0) {
                    if (timediff(time, observation.data[j].time) == 0.0) {
                        xp[0][np[0]] = x[0][n[0]];
                        yp[0][np[0]] = y[0][n[0]];
                        colp[0][np[0]] = observationColor(observation.data + j, azimuth[j], elevtion[j]);
                        if (np[0] < MAXSAT && colp[0][np[0]] != Qt::black) np[0]++;
                    }
                    if (n[0] < nObservation) n[0]++;
                }
                if (y[1][n[1]] != 0.0) {
                    if (elevtion[j] >= elevationMask * D2R) {
                        ave += y[1][n[1]];
                        rms += SQR(y[1][n[1]]);
                        nrms++;
                    }
                    if (timediff(time, observation.data[j].time) == 0.0) {
                        xp[1][np[1]] = x[1][n[1]];
                        yp[1][np[1]] = y[1][n[1]];
                        colp[1][np[1]] = observationColor(observation.data + j, azimuth[j], elevtion[j]);
                        if (np[1] < MAXSAT && colp[1][np[1]] != Qt::black) np[1]++;
                    }
                    if (n[1] < nObservation) n[1]++;
                }
            }
            if (!level || !(plotStyle % 2)) {
                for (int i = 0; i < 2; i++) {
                    if (!btn[i]->isChecked()) continue;
                    drawPolyS(graphE[i], c, x[i], y[i], n[i], cColor[3], 0);
                }
            }
            if (level && plotStyle < 2) {
                for (int i = 0; i < 2; i++) {
                    if (!btn[i]->isChecked()) continue;
                    for (int j = 0; j < n[i]; j++)
                        graphE[i]->drawMark(c, x[i][j], y[i][j], 0, col[i][j], markSize, 0);
                }
            }
        }
        for (int i = 0; i < 2; i++) {
            delete [] x[i];
            delete [] y[i];
            delete [] col[i];
        }
        if (btnShowTrack->isChecked() && 0 <= observationIndex && observationIndex < nObservation && btnSolution1->isChecked()) {
            for (int i = 0; i < 2; i++) {
                if (!btn[i]->isChecked()) continue;
                for (int j = 0; j < np[i]; j++) {
                    graphE[i]->drawMark(c, xp[i][j], yp[i][j], 0, cColor[0], markSize * 2 + 8, 0);
                    graphE[i]->drawMark(c, xp[i][j], yp[i][j], 1, cColor[2], markSize * 2 + 6, 0);
                    graphE[i]->drawMark(c, xp[i][j], yp[i][j], 0, colp[i][j], markSize * 2 + 2, 0);
                }
            }
        }
    }

    if (showStats) {
        int i;
        for (i = 0; i < 2; i++) if (btn[i]->isChecked()) break;
        if (i < 2) {
            QPoint p1,p2;
            int hh=(int)(lblDisplay->font().pixelSize()*1.5);

            graphE[i]->getPosition(p1, p2);
            p1.rx() += 8; p1.ry() += 6;
            s = QString("MARKER: %1 %2").arg(station.name).arg(station.marker);
            drawLabel(graphE[i], c, p1, s, 1, 2); p1.ry() += hh;
            s = QString("REC: %1 %2 %3").arg(station.rectype).arg(station.recver).arg(station.recsno);
            drawLabel(graphE[i], c, p1, s, 1, 2); p1.ry() += hh;
            s = QString("ANT: %1 %2").arg(station.antdes).arg(station.antsno);
            drawLabel(graphE[i], c, p1, s, 1, 2); p1.ry() += hh;
        }
        if (btn[1]->isChecked() && nrms > 0 && !btnShowTrack->isChecked()) {
            QPoint p1,p2;
            ave = ave / nrms;
            rms = SQRT(rms / nrms);
            graphE[1]->getPosition(p1, p2);
            p1.setX(p2.x() - 8); p1.ry() += 6;
            drawLabel(graphE[1], c, p1, QString("AVE=%1m RMS=%2m").arg(ave, 0, 'f', 4).arg(rms, 0, 'f', 4), 2, 2);
        }
    }
}
// draw MP-skyplot ----------------------------------------------------------
void Plot::drawMpS(QPainter &c, int level)
{
    QString obstype = cBObservationType2->currentText();
    double r,xl[2],yl[2],xs,ys;

    trace(3, "DrawSnrS: level=%d\n", level);

    graphS->getLimits(xl, yl);
    r = (xl[1] - xl[0] < yl[1] - yl[0] ? xl[1] - xl[0] : yl[1] - yl[0]) * 0.45;

    if (btnShowImage->isChecked())
        drawSkyImage(c, level);

    if (btnShowSkyplot->isChecked())
        graphS->drawSkyPlot(c, 0.0, 0.0, cColor[1], cColor[2], cColor[0], r * 2.0);

    if (!btnSolution1->isChecked() || nObservation <= 0 || simObservation) return;

    graphS->getScale(xs, ys);

    for (int sat = 1; sat <= MAXSAT; sat++) {
        double p[MAXSAT][2]={{0}};

        if (satelliteMask[sat - 1] || !satelliteSelection[sat - 1]) continue;

        for (int i = 0; i < observation.n; i++) {
            obsd_t *obs=observation.data+i;
            int j,freq;
            bool ok;

            if (obs->sat!=sat||elevtion[i]<=0.0) continue;

            freq = obstype.mid(1).toInt(&ok);

            if (ok) {
                j=freq-1;
            }
            else {                for (j=0;j<NFREQ+NEXOBS;j++) {
                    if (!strcmp(code2obs(obs->code[j]),qPrintable(obstype))) break;
                }
                if (j>=NFREQ+NEXOBS) continue;
            }
            double x=r*sin(azimuth[i])*(1.0-2.0*elevtion[i]/PI);
            double y=r*cos(azimuth[i])*(1.0-2.0*elevtion[i]/PI);
            double xp=p[sat-1][0];
            double yp=p[sat-1][1];
            QColor col=mpColor(!multipath[j]?0.0:multipath[j][i]);

            if ((x - xp) * (x - xp) + (y - yp) * (y - yp) >= xs * xs) {
                int siz = plotStyle < 2 ? markSize : 1;
                graphS->drawMark(c, x, y, 0, col, siz, 0);
                graphS->drawMark(c, x, y, 0, plotStyle < 2 ? col : cColor[3], siz, 0);
                p[sat - 1][0] = x;
                p[sat - 1][1] = y;
            }
        }
    }

    if (btnShowTrack->isChecked() && 0 <= observationIndex && observationIndex < nObservation) {
        for (int i=indexObservation[observationIndex];i<observation.n&&i<indexObservation[observationIndex+1];i++) {
            obsd_t *obs=observation.data+i;
            int j,freq;
            bool ok;

            freq=obstype.mid(1).toInt(&ok);

            if (ok) {
                j=freq-1;
            }
            else {
                for (j=0;j<NFREQ+NEXOBS;j++) {
                    if (!strcmp(code2obs(obs->code[j]),qPrintable(obstype))) break;
                }
                if (j>=NFREQ+NEXOBS) continue;
            }
            QColor col=mpColor(!multipath[j]?0.0:multipath[j][i]);
            double x = r * sin(azimuth[i]) * (1.0 - 2.0 * elevtion[i] / PI);
            double y = r * cos(azimuth[i]) * (1.0 - 2.0 * elevtion[i] / PI);

            char id[32];
            satno2id(obs->sat, id);
            graphS->drawMark(c, x, y, 0, col, lblDisplay->font().pointSize() * 2 + 5, 0);
            graphS->drawMark(c, x, y, 1, cColor[2], lblDisplay->font().pointSize() * 2 + 5, 0);
            graphS->drawText(c, x, y, QString(id), cColor[0], 0, 0, 0);
        }
    }
}
// draw residuals and SNR/elevation plot ------------------------------------
void Plot::drawResidual(QPainter &c, int level)
{
    QString label[] = {
        tr("Pseudorange Residuals (m)"),
        tr("Carrier-Phase Residuals (m)"),
        tr("Elevation Angle (deg) / Signal Strength (dBHz)")
    };
    QString str;
    QPushButton *btn[] = { btnOn1, btnOn2, btnOn3 };
    int sel = !btnSolution1->isChecked() && btnSolution2->isChecked() ? 1 : 0, ind = solutionIndex[sel];
    int frq = cBFrequencyType->currentIndex() + 1, n=solutionStat[sel].n;

    trace(3, "DrawRes: level=%d\n", level);

    if (0 <= ind && ind < solutionData[sel].n && btnShowTrack->isChecked() && btnFixHorizontal->isChecked()) {
        gtime_t t = solutionData[sel].data[ind].time;

        for (int i=0;i<3;i++) {
            double off,xc,yc,xl[2],yl[2];

            if (btnFixHorizontal->isChecked()) {
                graphG[i]->getLimits(xl, yl);
                off = Xcent * (xl[1] - xl[0]) / 2.0;
                graphG[i]->getCenter(xc, yc);
                graphG[i]->getCenter(xc, yc);
                graphG[i]->setCenter(timePosition(t) - off, yc);
            } else {
                graphG[i]->getRight(xc, yc);
                graphG[i]->setRight(timePosition(t), yc);
            }
        }
    }
    int j = -1;
    for (int i = 0; i < 3; i++) if (btn[i]->isChecked()) j = i;

    for (int i = 0; i < 3; i++) {
        if (!btn[i]->isChecked()) continue;
        graphG[i]->xLabelPosition = timeLabel ? (i == j ? 6 : 5) : (i == j ? 1 : 0);
        graphG[i]->week = week;
        graphG[i]->drawAxis(c, showLabel, showLabel);
    }

    if (n > 0 && ((sel == 0 && btnSolution1->isChecked()) || (sel == 1 && btnSolution2->isChecked()))) {
        QColor *col[4];
        double *x[4],*y[4],sum[2]={0},sum2[2]={0};
        int ns[2]={0};

        for (int i=0;i<4;i++) {
            x[i]=new double[n],
            y[i]=new double[n];
            col[i]=new QColor[n];
        }

        for (int sat = 1; sat <= MAXSAT; sat++) {
            if (satelliteMask[sat - 1] || !satelliteSelection[sat - 1]) continue;

            int m[4]={0};

            for (int i = 0; i < n; i++) {
                solstat_t *p = solutionStat[sel].data + i;
                int q;

                if (p->sat != sat || p->frq != frq) continue;
                if (p->resp == 0.0 && p->resc == 0.0) continue;
                x[0][m[0]]=x[1][m[1]]=x[2][m[2]]=x[3][m[3]]= timePosition(p->time);
                y[0][m[0]] = p->resp;
                y[1][m[1]] = p->resc;
                y[2][m[2]] = p->el * R2D;
                y[3][m[3]] = p->snr * SNR_UNIT;

                if (!(p->flag >> 5)) q = 0;          // invalid
                else if ((p->flag & 7) <= 1) q = 2;  // float
                else if ((p->flag & 7) <= 3) q = 1;  // fixed
                else q = 6;                          // ppp

                col[0][m[0]]=mColor[0][q];
                col[1][m[1]]=((p->flag>>3)&1)?Qt::red:mColor[0][q];
                col[2][m[2]]=mColor[0][1];
                col[3][m[3]]=mColor[0][4];        // slip

                if (p->resp != 0.0) {
                    sum [0] += p->resp;
                    sum2[0] += p->resp * p->resp;
                    ns[0]++;
                }
                if (p->resc != 0.0) {
                    sum [1] += p->resc;
                    sum2[1] += p->resc * p->resc;
                    ns[1]++;
                }
                m[0]++; m[1]++; m[2]++; m[3]++;
            }
            for (int i = 0; i < 3; i++) {
                if (!btn[i]->isChecked()) continue;
                if (!level || !(plotStyle % 2)) {
                    drawPolyS(graphG[i], c, x[i], y[i], m[i], cColor[3], 0);
                    if (i == 2) drawPolyS(graphG[i], c, x[3], y[3], m[3], cColor[3], 0);
                }
                if (level && plotStyle < 2) {

                    for (int j=0;j<m[i];j++) {
                        graphG[i]->drawMark(c, x[i][j],y[i][j],0,col[i][j],markSize,0);
                    }
                    if (i==2) {
                        for (int j=0;j<m[3];j++) {
                            graphG[i]->drawMark(c, x[3][j],y[3][j],0,col[3][j],markSize,0);
                        }
                    }
                }
            }
        }
        for (int i=0;i<4;i++) {
            delete [] x[i];
            delete [] y[i];
            delete [] col[i];
        }

        if (showStats) {
            for (int i = 0; i < 2; i++) {
                if (!btn[i]->isChecked()) continue;
                QPoint p1,p2;

                double ave, std, rms;
                ave = ns[i] <= 0 ? 0.0 : sum[i] / ns[i];
                std = ns[i] <= 1 ? 0.0 : SQRT((sum2[i] - 2.0 * sum[i] * ave + ns[i] * ave * ave) / (ns[i] - 1));
                rms = ns[i] <= 0 ? 0.0 : SQRT(sum2[i] / ns[i]);
                graphG[i]->getPosition(p1, p2);
                p1.setX(p2.x() - 5);
                p1.ry() += 3;
                str = tr("AVE=%1m STD=%2m RMS=%3m").arg(ave, 0, 'f', 3).arg(std, 0, 'f', 3).arg(rms, 0, 'f', 3);
                drawLabel(graphG[i], c, p1, str, 2, 2);
            }
        }
        if (btnShowTrack->isChecked() && 0 <= ind && ind < solutionData[sel].n && (btnSolution1->isChecked() || btnSolution2->isChecked())) {
            for (int i = 0, j = 0; i < 3; i++) {
                if (!btn[i]->isChecked()) continue;
                gtime_t t = solutionData[sel].data[ind].time;
                double xl[2], yl[2];
                graphG[i]->getLimits(xl, yl);
                xl[0] = xl[1] = timePosition(t);
                graphG[i]->drawPoly(c, xl, yl, 2, ind == 0 ? cColor[1] : cColor[2], 0);
                if (j++ == 0) {
                    graphG[i]->drawMark(c, xl[0], yl[1] - 1E-6, 0, cColor[2], 5, 0);
                    graphG[i]->drawMark(c, xl[0], yl[1] - 1E-6, 1, cColor[2], 9, 0);
                }
            }
        }
    }
    for (int i = 0; i < 3; i++) {
        if (!btn[i]->isChecked()) continue;
        QPoint p1, p2;
        graphG[i]->getPosition(p1, p2);
        p1.rx() += 5; p1.ry() += 3;
        drawLabel(graphG[i], c, p1, label[i], 1, 2);
    }
}
// draw residuals - elevation plot ------------------------------------------
void Plot::drawResidualE(QPainter &c, int level)
{
    QPushButton *btn[]={btnOn1, btnOn2, btnOn3};
    QString label[]={"Pseudorange Residuals (m)","Carrier-Phase Residuals (m)"};
    int j,sel=!btnSolution1->isChecked()&&btnSolution2->isChecked()?1:0;
    int frq=cBFrequencyType->currentIndex()+1,n=solutionStat[sel].n;

    trace(3,"DrawResE: level=%d\n",level);

    j=0;
    for (int i=0;i<2;i++) if (btn[i]->isChecked()) j=i;
    for (int i=0;i<2;i++) {
        if (!btn[i]->isChecked()) continue;

        QPoint p1,p2;
        double xl[2]={-0.001,90.0};
        double yl[2][2]={{-maxMP,maxMP},{-maxMP/100.0,maxMP/100.0}};
        
        graphE[i]->xLabelPosition=i==j?1:0;
        graphE[i]->yLabelPosition=1;
        graphE[i]->setLimits(xl,yl[i]);
        graphE[i]->setTick(0.0,0.0);
        graphE[i]->drawAxis(c, 1,1);
        graphE[i]->getPosition(p1,p2);
        p1.setX(lblDisplay->font().pixelSize());
        p1.setY((p1.y()+p2.y())/2);
        graphE[i]->drawText(c, p1,label[i],cColor[2],0,0,90);
        if (i==j) {
            p2.rx()-=8; p2.ry()-=6;
            graphE[i]->drawText(c, p2,"Elevation ( ° )",cColor[2],2,1,0);
        }
    }
    if (n>0&&((sel==0&&btnSolution1->isChecked())||(sel==1&&btnSolution2->isChecked()))) {
        QColor *col[2];
        double *x[2],*y[2],sum[2]={0},sum2[2]={0};
        int ns[2]={0};

        for (int i=0;i<2;i++) {
            x  [i]=new double[n],
            y  [i]=new double[n];
            col[i]=new QColor[n];
        }
        for (int sat=1;sat<=MAXSAT;sat++) {
            if (satelliteMask[sat-1]||!satelliteSelection[sat-1]) continue;
            int q,m[2]={0};

            for (int i=0;i<n;i++) {
                solstat_t *p=solutionStat[sel].data+i;
                if (p->sat!=sat||p->frq!=frq) continue;

                x[0][m[0]]=x[1][m[1]]=p->el*R2D;
                y[0][m[0]]=p->resp;
                y[1][m[1]]=p->resc;
                if      (!(p->flag>>5))  q=0; // invalid
                else if ((p->flag&7)<=1) q=2; // float
                else if ((p->flag&7)<=3) q=1; // fixed
                else                     q=6; // ppp

                col[0][m[0]]=mColor[0][q];
                col[1][m[1]]=((p->flag>>3)&1)?Qt::red:mColor[0][q];

                if (p->resp!=0.0) {
                    sum [0]+=p->resp;
                    sum2[0]+=p->resp*p->resp;
                    ns[0]++;
                }
                if (p->resc!=0.0) {
                    sum [1]+=p->resc;
                    sum2[1]+=p->resc*p->resc;
                    ns[1]++;
                }
                m[0]++; m[1]++;
            }
            for (int i=0;i<2;i++) {
                if (!btn[i]->isChecked()) continue;
                if (!level||!(plotStyle%2)) {
                    drawPolyS(graphE[i],c,x[i],y[i],m[i],cColor[3],0);
                }
                if (level&&plotStyle<2) {
                    for (int j=0;j<m[i];j++) {
                        graphE[i]->drawMark(c, x[i][j],y[i][j],0,col[i][j],markSize,0);
                    }
                }
            }
        }
        for (int i=0;i<2;i++) {
            delete [] x[i];
            delete [] y[i];
            delete [] col[i];
        }
        if (showStats) {
            for (int i=0;i<2;i++) {
                if (!btn[i]->isChecked()) continue;

                QString str;
                QPoint p1,p2;
                double ave,std,rms;

                ave=ns[i]<=0?0.0:sum[i]/ns[i];
                std=ns[i]<=1?0.0:SQRT((sum2[i]-2.0*sum[i]*ave+ns[i]*ave*ave)/(ns[i]-1));
                rms=ns[i]<=0?0.0:SQRT(sum2[i]/ns[i]);
                graphE[i]->getPosition(p1,p2);
                p1.setX(p2.x()-5);
                p1.ry()+=3;
                str= QString("AVE=%1m STD=%2m RMS=%3m").arg(ave,0,'f',3).arg(std,0,'f',3).arg(rms,0,'f',3);
                drawLabel(graphG[i],c,p1,str,2,2);
            }
        }
    }
}
// draw polyline without time-gaps ------------------------------------------
void Plot::drawPolyS(Graph *graph, QPainter &c, double *x, double *y, int n,
             const QColor &color, int style)
{
    int i, j;

    for (i = 0; i < n; i = j) {
        for (j = i + 1; j < n; j++) if (fabs(x[j] - x[j - 1]) > TBRK) break;
        graph->drawPoly(c, x + i, y + i, j - i, color, style);
    }
}
// draw label with hemming --------------------------------------------------
void Plot::drawLabel(Graph *g, QPainter &c, const QPoint &p, const QString &label, int ha, int va)
{
    g->drawText(c, p, label, cColor[2], cColor[0], ha, va, 0);
}
// draw mark with hemming ---------------------------------------------------
void Plot::drawMark(Graph *g, QPainter &c, const QPoint &p, int mark, const QColor &color,
            int size, int rot)
{
    g->drawMark(c, p, mark, color, cColor[0], size, rot);
}
// refresh map view --------------------------------------------------
void Plot::refresh_MapView(void)
{
    sol_t *sol;
    double pos[3] = { 0 };

    if (btnShowTrack->isChecked()) {
        if (btnSolution2->isChecked() && solutionData[1].n>0&&
            (sol=getsol(solutionData+1,solutionIndex[1]))) {
            ecef2pos(sol->rr, pos);
            mapView->setMark(2,pos[0]*R2D,pos[1]*R2D);
            mapView->showMark(2);

        } else {
            mapView->hideMark(2);
        }
        if (btnSolution1->isChecked() && solutionData[0].n > 0 &&
                (sol=getsol(solutionData,solutionIndex[0]))) {
            ecef2pos(sol->rr, pos);
            mapView->setMark(1, pos[0]*R2D,pos[1]*R2D);
            mapView->showMark(1);
        } else {
            mapView->hideMark(1);
        }
    } else {
        mapView->hideMark(1);
        mapView->hideMark(2);
    }

}
