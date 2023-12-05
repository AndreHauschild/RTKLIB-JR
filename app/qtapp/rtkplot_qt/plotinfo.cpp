//---------------------------------------------------------------------------
// plotinfo.c: rtkplot info functions
//---------------------------------------------------------------------------
#include "plotmain.h"

#include "rtklib.h"

#define ATAN2(x, y)  ((x) * (x) + (y) * (y) > 1E-12 ? atan2(x, y) : 0.0)

// update information on status-bar -----------------------------------------
void Plot::updateInfo(void)
{
    int showobs = (PLOT_OBS <= plotType && plotType <= PLOT_DOP) ||
              plotType == PLOT_SNR || plotType == PLOT_SNRE || plotType == PLOT_MPS;

    trace(3, "UpdateInfo:\n");

    if (btnShowTrack->isChecked()) {
        if (showobs) updateTimeObservation(); else updateTimeSolution();
    } else {
        if (showobs) updateInfoObservation(); else updateInfoSolution();
    }
}
// update time-information for observation-data plot ------------------------
void Plot::updateTimeObservation(void)
{
    QString msgs1[] = { " #FRQ=5 ", " 4 ", " 3 ", " 2 ", " 1", "", "" };
    QString msgs2[] = { " SNR=...45.", "..40.", "..35.", "..30.", "..25 ", "", " <25 " };
    QString msgs3[] = { " SYS=GPS ", "GLO ", "GAL ", "QZS ", "BDS ", "IRN ", "SBS" };
    QString msgs4[] = { " MP=..0.6", "..0.3", "..0.0..", "-0.3..", "-0.6..", "", "" };
    QString msgs[8] = {"", "", "", "", "", "", "", ""};
    QString msg;
    double azel[MAXOBS * 2], dop[4] = { 0 };
    int i, ns = 0, no = 0, ind = observationIndex;
    QString tstr;

    trace(3, "UpdateTimeObs\n");

    if (btnSolution1->isChecked() && 0 <= ind && ind < nObservation) {
        for (i = indexObservation[ind]; i < observation.n && i < indexObservation[ind + 1]; i++, no++) {
            if (satelliteMask[observation.data[i].sat - 1] || !satelliteSelection[observation.data[i].sat - 1]) continue;
            if (elevtion[i] < elevationMask * D2R) continue;
            if (elevationMaskP && elevtion[i] < elevationMaskData[static_cast<int>(azimuth[i] * R2D + 0.5)]) continue;
            azel[ns * 2] = azimuth[i];
            azel[1 + ns * 2] = elevtion[i];
            ns++;
        }
    }
    if (ns >= 0) {
        dops(ns, azel, elevationMask * D2R, dop);

        timeStream(observation.data[indexObservation[ind]].time, 3, 1, tstr);
        msg = QString("[1]%1 : N=%2 ").arg(tstr).arg(no);

        if (plotType == PLOT_DOP) {
            msgs[0] = (QString("NSAT=%1").arg(ns));
            msgs[1] = (QString(" GDOP=%1").arg(dop[0], 0, 'f', 1));
            msgs[2] = (QString(" PDOP=%1").arg(dop[1], 0, 'f', 1));
            msgs[3] = (QString(" HDOP=%1").arg(dop[2], 0, 'f', 1));
            msgs[4] = (QString(" VDOP=%1").arg(dop[3], 0, 'f', 1));
        } else if (plotType <= PLOT_SKY && cBObservationType->currentIndex() == 0) {
            msg += QString("NSAT=%1 ").arg(ns);
            for (i = 0; i < 7; i++) msgs[i] = simObservation ? msgs3[i] : msgs1[i];
        } else if (plotType == PLOT_MPS) {
            msg += QString("NSAT=%1 ").arg(ns);
            for (i = 0; i < 7; i++) msgs[i] = msgs4[i];
        } else {
            msg += QString("NSAT=%1 ").arg(ns);
            for (i = 0; i < 7; i++) msgs[i] = simObservation ? msgs3[i] : msgs2[i];
        }
    }
    showMessage(msg);
    showLegend(msgs);
}
// update time-information for solution plot --------------------------------
void Plot::updateTimeSolution(void)
{
    const char *unit[] = { "m", "m/s", "m/s2" }, *u;
    const QString sol[] = { tr(""), tr("FIX"), tr("FLOAT"), tr("SBAS"), tr("DGPS"), tr("Single"), tr("PPP") };
    QString msg;
    QString msgs[8] = {"", "", "", "", "", "", "", ""};
    sol_t *data;
    double pos[3], r, az, el;
    int sel = btnSolution1->isChecked() || !btnSolution2->isChecked() ? 0 : 1, ind = solutionIndex[sel];
    QString tstr;

    trace(3, "UpdateTimeSol\n");

    if ((btnSolution1->isChecked() || btnSolution2->isChecked() || btnSolution12->isChecked()) &&
        (data = getsol(solutionData + sel, ind))) {
        if (!connectState) msg = QString("[%1]").arg(sel + 1); else msg = "[R]";

        timeStream(data->time, 2, 1, tstr);
        msg += tstr;
        msg += " : ";

        if (PLOT_SOLP <= plotType && plotType <= PLOT_SOLA) {
            TIMEPOS *p=solutionToPosition(solutionData+sel,ind,0,plotType-PLOT_SOLP);
            u = unit[plotType - PLOT_SOLP];
            msg += QString("E=%1%2 N=%3%2 U=%4%2 Q=")
                   .arg(p->x[0], 7, 'f', 4).arg(u).arg(p->y[0], 7, 'f', 4).arg(p->z[0], 7, 'f', 4);
            delete p;
        } else if (plotType == PLOT_NSAT) {
            msg += QString("NS=%1 AGE=%2 RATIO=%3 Q=").arg(data->ns).arg(data->age, 0, 'f', 1)
                   .arg(data->ratio, 0, 'f', 1);
        } else if (!data->type) {
            ecef2pos(data->rr, pos);
            msg += latLonString(pos, 9) + QString(" %1m  Q=").arg(pos[2], 9, 'f', 4);
        } else {
            r = norm(data->rr, 3);
            az = norm(data->rr, 2) <= 1E-12 ? 0.0 : atan2(data->rr[0], data->rr[1]) * R2D;
            el = r <= 1E-12 ? 0.0 : asin(data->rr[2] / r) * R2D;
            msg += QString("B=%1m D=%2%3 %4%5  Q=")
                   .arg(r, 0, 'f', 3).arg(az < 0.0 ? az + 360.0 : az, 6, 'f', 2).arg(degreeChar).arg(el, 5, 'f', 2).arg(degreeChar);
        }
        if (1 <= data->stat && data->stat <= 6)
            msgs[data->stat - 1] = QString("%1:%2").arg(data->stat).arg(sol[data->stat]);
    }
    showMessage(msg);
    showLegend(msgs);
}
// update statistics-information for observation-data plot ------------------
void Plot::updateInfoObservation(void)
{
    QString msgs0[] = { " NSAT", " GDOP", " PDOP", " HDOP", " VDOP", "", "" };
    QString msgs1[] = { " #FRQ=5 ", " 4 ", " 3 ", " 2 ", " 1 ", "", "" };
    QString msgs2[] = { " SNR=...45.", "..40.", "..35.", "..30.", "..25 ", "", " <25 " };
    QString msgs3[] = { " SYS=GPS ", "GLO ", "GAL ", "QZS ", "BDS ", "IRN ", "SBS" };
    QString msgs4[] = { " MP=..0.6", "..0.3", "..0.0..", "-0.3..", "-0.6..", "", "" };
    QString msg;
    QString msgs[8] = {"", "", "", "", "", "", "", ""};
    gtime_t ts = { 0, 0 }, te = { 0, 0 }, t, tp = { 0, 0 };
    int i, n = 0, ne = 0, p;
    QString s1, s2;

    trace(3, "UpdateInfoObs:\n");

    if (btnSolution1->isChecked()) {
        for (i = 0; i < observation.n; i++) {
            t = observation.data[i].time;
            if (ts.time == 0) ts = t;
            te = t;
            if (tp.time == 0 || timediff(t, tp) > TTOL) ne++;
            n++; tp = t;
        }
    }
    if (n > 0) {
        timeStream(ts, 0, 0, s1);
        timeStream(te, 0, 1, s2);
        if (timeLabel&&(p=s1.indexOf(' '))) s1=s1.left(p);
        msg = QString("[1]%1-%2 : EP=%3 N=%4").arg(s1).arg(s2.mid(timeLabel ? 5 : 0)).arg(ne).arg(n);

        for (i = 0; i < 7; i++) {
            if (plotType == PLOT_DOP)
                msgs[i] = msgs0[i];
            else if (plotType <= PLOT_SKY && cBObservationType->currentIndex() == 0)
                msgs[i] = simObservation ? msgs3[i] : msgs1[i];
            else if (plotType == PLOT_MPS)
                msgs[i] = msgs4[i];
            else
                msgs[i] = simObservation ? msgs3[i] : msgs2[i];
        }
    }
    showMessage(msg);
    showLegend(msgs);
}
// update statistics-information for solution plot --------------------------
void Plot::updateInfoSolution(void)
{
    QString msg, msgs[8] = {"", "", "", "", "", "", "", ""}, s;
    TIMEPOS *pos = NULL, *pos1, *pos2;
    sol_t *data;
    gtime_t ts = { 0, 0 }, te = { 0, 0 };
    double r[3], b, bl[2] = { 1E9, 0.0 };
    int i, j,p, n = 0, nq[8] = { 0 }, sel = btnSolution1->isChecked() || !btnSolution2->isChecked() ? 0 : 1;
    QString s1, s2;

    trace(3, "UpdateInfoSol:\n");

    if (btnSolution1->isChecked() || btnSolution2->isChecked()) {
        pos = solutionToPosition(solutionData + sel, -1, 0, 0);
    } else if (btnSolution12->isChecked()) {
        pos1 = solutionToPosition(solutionData, -1, 0, 0);
        pos2 = solutionToPosition(solutionData + 1, -1, 0, 0);
        pos = pos1->diff(pos2, 0);
        delete pos1;
        delete pos2;
    }
    if (pos) {
        for (i = 0; i < pos->n; i++) {
            if (ts.time == 0) ts = pos->t[i];
            te = pos->t[i];
            nq[pos->q[i]]++;
            n++;
        }
        delete pos;
    }
    for (i = 0; (data = getsol(solutionData + sel, i)) != NULL; i++) {
        if (data->type) {
            b = norm(data->rr, 3);
        } else if (norm(solutionData[sel].rb, 3) > 0.0) {
            for (j = 0; j < 3; j++) r[j] = data->rr[j] - solutionData[sel].rb[j];
            b = norm(r, 3);
        } else {
            b = 0.0;
        }
        if (b < bl[0]) bl[0] = b;
        if (b > bl[1]) bl[1] = b;
    }
    if (n > 0) {
        if (!connectState) msg = QString("[%1]").arg(sel + 1); else msg = "[R]";

        timeStream(ts, 0, 0, s1);
        timeStream(te, 0, 1, s2);
        if (timeLabel&&(p=s1.indexOf(' '))) s1=s1.left(p);
        msg += QString("%1-%2 : N=%3").arg(s1).arg(s2.mid(timeLabel ? 5 : 0)).arg(n);

        if (bl[0] + 100.0 < bl[1])
            msg += QString(" B=%1-%2km").arg(bl[0] / 1E3, 0, 'f', 1).arg(bl[1] / 1E3, 0, 'f', 1);
        else
            msg += QString(" B=%1km").arg(bl[0] / 1E3, 0, 'f', 1);
        msg += " Q=";

        for (i = 1; i <= 6; i++) {
            if (nq[i] <= 0) continue;
            msgs[i - 1] = QString("%1:%2(%3%) ").arg(i).arg(nq[i]).arg(static_cast<double>(nq[i]) / n * 100.0, 0, 'f', 1);
        }
    }
    showMessage(msg);
    showLegend(msgs);
}
// update plot-type pull-down menu ------------------------------------------
void Plot::updatePlotType(void)
{
    int i;

    trace(3, "UpdatePlotType\n");

    cBPlotTypeS->blockSignals(true);
    cBPlotTypeS->clear();
    if (solutionData[0].n > 0 || solutionData[1].n > 0 ||
        (nObservation <= 0 && solutionStat[0].n <= 0 && solutionStat[1].n <= 0)) {
        cBPlotTypeS->addItem(PTypes[PLOT_TRK]);
        cBPlotTypeS->addItem(PTypes[PLOT_SOLP]);
        cBPlotTypeS->addItem(PTypes[PLOT_SOLV]);
        cBPlotTypeS->addItem(PTypes[PLOT_SOLA]);
        cBPlotTypeS->addItem(PTypes[PLOT_NSAT]);
    }
    if (nObservation > 0) {
        cBPlotTypeS->addItem(PTypes[PLOT_OBS]);
        cBPlotTypeS->addItem(PTypes[PLOT_SKY]);
        cBPlotTypeS->addItem(PTypes[PLOT_DOP]);
    }
    if (solutionStat[0].n > 0 || solutionStat[1].n > 0) {
        cBPlotTypeS->addItem(PTypes[PLOT_RES]);
        cBPlotTypeS->addItem(PTypes[PLOT_RESE]);
    }
    if (nObservation > 0) {
        cBPlotTypeS->addItem(PTypes[PLOT_SNR]);
        cBPlotTypeS->addItem(PTypes[PLOT_SNRE]);
        cBPlotTypeS->addItem(PTypes[PLOT_MPS]);
    }
    for (i = 0; i < cBPlotTypeS->count(); i++) {
        if (cBPlotTypeS->itemText(i) != PTypes[plotType]) continue;
        cBPlotTypeS->setCurrentIndex(i);
        cBPlotTypeS->blockSignals(false);
        return;
    }
    cBPlotTypeS->setCurrentIndex(0);

    cBPlotTypeS->blockSignals(false);
}
// update satellite-list pull-down menu -------------------------------------
void Plot::updateSatelliteList(void)
{
    int i, j, sys, sysp = 0, sat, smask[MAXSAT] = { 0 };
    char s[8];

    trace(3, "UpdateSatList\n");

    for (i = 0; i < 2; i++) for (j = 0; j < solutionStat[i].n; j++) {
            sat = solutionStat[i].data[j].sat;
            if (1 <= sat && sat <= MAXSAT) smask[sat - 1] = 1;
        }
    for (j = 0; j < observation.n; j++) {
        sat = observation.data[j].sat;
        if (1 <= sat && sat <= MAXSAT) smask[sat - 1] = 1;
    }
    cBSatelliteList->clear();
    cBSatelliteList->addItem("ALL");

    for (sat = 1; sat <= MAXSAT; sat++) {
        if (satelliteMask[sat - 1] || !smask[sat - 1]) continue;
        if ((sys = satsys(sat, NULL)) == sysp) continue;
        switch ((sysp = sys)) {
        case SYS_GPS: strcpy(s, "G"); break;
        case SYS_GLO: strcpy(s, "R"); break;
        case SYS_GAL: strcpy(s, "E"); break;
        case SYS_QZS: strcpy(s, "J"); break;
        case SYS_CMP: strcpy(s, "C"); break;
        case SYS_IRN: strcpy(s, "I"); break;
        case SYS_SBS: strcpy(s, "S"); break;
        }
        cBSatelliteList->addItem(s);
    }
    for (sat = 1; sat <= MAXSAT; sat++) {
        if (satelliteMask[sat - 1] || !smask[sat - 1]) continue;
        satno2id(sat, s);
        cBSatelliteList->addItem(s);
    }
    cBSatelliteList->setCurrentIndex(0);

    updateSatelliteSelection();
}
// string compare --------------------------------------------------------------
static int _strcmp(const void *str1, const void *str2)
{
    return strcmp(*(const char **)str1, *(const char **)str2);
}
// update observation type pull-down menu --------------------------------------
void Plot::updateObservationType(void)
{
    char *codes[MAXCODE + 1], *obs;
    int i, j, n = 0, cmask[MAXCODE + 1] = { 0 };

    trace(3, "UpdateObsType\n");

    for (i = 0; i < observation.n; i++) for (j = 0; j < NFREQ + NEXOBS; j++)
            cmask[observation.data[i].code[j]] = 1;

    for (unsigned char c = 1; c <= MAXCODE; c++) {
        if (!cmask[c]) continue;
        if (!*(obs=code2obs((uint8_t)i))) continue;
        codes[n++]=obs;
    }
    qsort(codes,n,sizeof(char *),_strcmp);
    cBObservationType->clear();
    cBObservationType2->clear();
    cBObservationType->addItem(tr("ALL"));

    for (i = 0; i < NFREQ; i++) {
        cBObservationType->addItem(QString("L%1").arg(i+1));
        cBObservationType2->addItem(QString("L%1").arg(i+1));
    }
    for (i = 0; i < n; i++) {
        cBObservationType->addItem(QString("L%1").arg(codes[i]));
        cBObservationType2->addItem(QString("L%1").arg(codes[i]));
    }
    cBObservationType->setCurrentIndex(0);
    cBObservationType2->setCurrentIndex(0);
}
// update information for current-cursor position ---------------------------
void Plot::updatePoint(int x, int y)
{
    gtime_t time;
    QPoint p(x, y);
    double enu[3] = { 0 }, rr[3], pos[3], xx, yy, r, xl[2], yl[2], q[2], az, el;
    int i;
    QString tstr;
    QString msg;

    trace(4, "UpdatePoint: x=%d y=%d\n", x, y);

    if (plotType == PLOT_TRK) { // track-plot
        graphT->toPos(p, enu[0], enu[1]);

        if (pointType == 1 || norm(oPosition, 3) <= 0.0) {
            msg = QString("E:%1 m N:%2 m").arg(enu[0], 0, 'f', 4).arg(enu[1], 0, 'f', 4);
        } else if (pointType == 2) {
            r = norm(enu, 2);
            az = r <= 0.0 ? 0.0 : ATAN2(enu[0], enu[1]) * R2D;
            if (az < 0.0) az += 360.0;
            msg = QString("R:%1 m D:%2%3").arg(r, 0, 'f', 4).arg(az, 5, 'f', 5).arg(degreeChar);
        } else {
            ecef2pos(oPosition, pos);
            enu2ecef(pos, enu, rr);
            for (i = 0; i < 3; i++) rr[i] += oPosition[i];
            ecef2pos(rr, pos);
            msg = latLonString(pos, 8);
        }
    } else if (plotType == PLOT_SKY || plotType == PLOT_MPS) { // sky-plot
        graphS->getLimits(xl, yl);
        graphS->toPos(p, q[0], q[1]);
        r = (xl[1] - xl[0] < yl[1] - yl[0] ? xl[1] - xl[0] : yl[1] - yl[0]) * 0.45;

        if ((el = 90.0 - 90.0 * norm(q, 2) / r) > 0.0) {
            az = el >= 90.0 ? 0.0 : ATAN2(q[0], q[1]) * R2D;
            if (az < 0.0) az += 360.0;
            msg = QString("AZ=%1%2 EL=%3%4").arg(az, 5, 'f', 1).arg(degreeChar).arg(el, 4, 'f', 1).arg(degreeChar);
        }
    } else if (plotType == PLOT_SNRE||plotType==PLOT_RESE) { // snr-el-plot
        graphE[0]->toPos(p, q[0], q[1]);
        msg = QString("EL=%1%2").arg(q[0], 4, 'f', 1).arg(degreeChar);
    } else {
        graphG[0]->toPos(p, xx, yy);
        time = gpst2time(week, xx);
        if (timeLabel == 2) time = utc2gpst(time);                              // UTC
        else if (timeLabel == 3) time = timeadd(gpst2utc(time), -9 * 3600.0);   // JST
        timeStream(time, 0, 1, tstr);
        msg = tstr;
    }
    lblMessage2->setVisible(true);
    lblMessage2->setText(msg);
}
//---------------------------------------------------------------------------
