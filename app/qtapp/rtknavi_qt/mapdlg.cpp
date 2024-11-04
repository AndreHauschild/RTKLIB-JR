//---------------------------------------------------------------------------

#include "rtklib.h"
#include "navimain.h"
#include "pntdlg.h"
#include "mapdlg.h"
#include "helper.h"

#include <QShowEvent>
#include <QMouseEvent>
#include <QPainter>

#include "ui_mapdlg.h"


//---------------------------------------------------------------------------

#define CHARDEG		"\260"			// character code of degree
#define SOLSIZE		13				// solution cycle size on map
#define PNTSIZE		10				// point cycle size on map
#define HISSIZE		4				// history cycle size on map
#define VELSIZE		40				// velocity cycle size on map
#define GRPHEIGHT	70				// vertical graph height
#define GRPHMARGIN	60				// vertical graph horizontal margin

//---------------------------------------------------------------------------
 MapDialog::MapDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::MapDialog)
{
    ui->setupUi(this);
    scale=6;
    pntIndex=0;

    for (int i = 0; i < 3; i++) {
        referencePosition[i] = currentPosition[i] = centerPosition[i] = 0.0;
	}
    referenceName = "Start Point";

    connect(ui->btnClose,SIGNAL(clicked(bool)),this,SLOT(accept()));
    connect(ui->btnCenter,SIGNAL(clicked(bool)),this,SLOT(BtnCenterClick()));
    connect(ui->btnExpand,SIGNAL(clicked(bool)),this,SLOT(BtnExpandClick()));
    connect(ui->btnPnt,SIGNAL(clicked(bool)),this,SLOT(BtnPntClick()));
    connect(ui->btnPntDlg,SIGNAL(clicked(bool)),this,SLOT(BtnPntDlgClick()));
    connect(ui->btnShrink,SIGNAL(clicked(bool)),this,SLOT(BtnShrinkClick()));
    connect(ui->btnTrack,SIGNAL(clicked(bool)),this,SLOT(BtnTrackClick()));
}
//---------------------------------------------------------------------------
void MapDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    updatePntList();
    updateEnable();
}
//---------------------------------------------------------------------------
void MapDialog::shrinkMap()
{
    if (scale < MAXSCALE) scale++;

    mainForm->updateMap();
}
//---------------------------------------------------------------------------
void MapDialog::expandMap()
{
    if (scale > 0) scale--;

    mainForm->updateMap();
}
//---------------------------------------------------------------------------
void MapDialog::btnPntClick()
{
    updateEnable();
}
//---------------------------------------------------------------------------
void MapDialog::pntListChange()
{
    pntIndex = ui->cBPntList->currentIndex();

    mainForm->updateMap();
}
//---------------------------------------------------------------------------
void MapDialog::centerMap()
{
    if (!ui->btnCenter->isChecked()) {
        for (int i = 0; i < 3; i++)
            centerPosition[i] = currentPosition[i];
	}
    mainForm->updateMap();
}
//---------------------------------------------------------------------------
void MapDialog::btnTrackClick()
{
    mainForm->updateMap();
}
//---------------------------------------------------------------------------
void MapDialog::btnPntDlgClick()
{
    PntDialog pntDialog(this);

    pntDialog.move(pos().x() + width() / 2 - pntDialog.width() / 2,
                   pos().y() + height() / 2 - pntDialog.height() / 2);

    pntDialog.exec();
    if (pntDialog.result()!=QDialog::Accepted) return;

    updatePntList();
    mainForm->updateMap();
}
//---------------------------------------------------------------------------
void MapDialog::FormResize()
{
    canvas=QPixmap(ui->display->width(), ui->display->height());
    mainForm->updateMap();
}
//---------------------------------------------------------------------------
void MapDialog::mousePressEvent(QMouseEvent *event)
{
    if (ui->btnCenter->isChecked() || !event->modifiers().testFlag(Qt::ShiftModifier)) return;
    dragging = 1;
    dragX0 = event->position().x();
    dragY0 = event->position().y();
    for (int i = 0; i < 3; i++) centerPositionDrag0[i] = centerPosition[i];
    setCursor(Qt::SizeAllCursor);
}
//---------------------------------------------------------------------------
void MapDialog::mouseMoveEvent(QMouseEvent* event)
{
    double sc[] = {
        0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000
	};
    double rr[3], posr[3], enu[3], fact = 40.0/sc[scale];
    if (!dragging) return;
    enu[0] = (dragX0-event->position().x()) / fact;
    enu[1] = (event->position().y()-dragY0) / fact;
    enu[2] = 0.0;
    ecef2pos(referencePosition, posr);
    enu2ecef(posr, enu, rr);
    for (int i = 0; i < 3; i++) centerPosition[i] = centerPositionDrag0[i] + rr[i];

    mainForm->updateMap();
}
//---------------------------------------------------------------------------
void MapDialog::mouseReleaseEvent(QMouseEvent*)
{
    if (dragging) setCursor(Qt::ArrowCursor);
    dragging = 0;
}
//---------------------------------------------------------------------------
void MapDialog::DispPaint()
{
    ui->display->setPixmap(canvas);
}
//---------------------------------------------------------------------------
void MapDialog::resetReference(void)
{
    referenceName = "Start Point";
    updatePntList();
    for (int i = 0; i < 3; i++) referencePosition[i] = 0.0;
}
//---------------------------------------------------------------------------
void MapDialog::updateMap(const double *sol, const double *ref,
	const double *vel, const int *stat, int psol, int psols, int psole,
    int nsol, QString *solstr, int currentstat)
{
    QColor color[] = {Qt::white, Qt::green, Color::Orange, Color::Fuchsia, Qt::blue, Qt::red, Color::Teal};
    QPainter c(&canvas);
    QString s;
    QPoint p0;
    double pos[3], posr[3], rr[3], enu[3], enuv[3], dir, *pntpos;
    int gint[] = {20, 20, 16, 20, 20, 16, 20, 20, 16, 20, 20, 16, 20, 20, 16, 20, 20, 16, 20};
    int ng[] = {10, 5, 5, 10, 5, 5, 10, 5, 5, 10, 5, 5, 10, 5, 5, 10, 5, 5, 10};
    QColor colsol = color[stat[psol]];

    QRect r(0, 0, canvas.width(), canvas.height());
    c.setBrush(Qt::white);
    c.fillRect(r, QBrush(Qt::white));
	
    for (int i = 0; i < 3; i++) {
        currentPosition[i] = sol[i+psol*3];
	}
    if (norm(referencePosition,3) <= 0.0) {
        if (ref&&norm(ref+psol, 3) > 0.0) {
            referenceName = "Base Station";
            for (int i = 0; i < 3; i++) referencePosition[i] = ref[i+psol*3];
            updatePntList();
        } else  {
            for (int i = 0; i < 3; i++) referencePosition[i] = currentPosition[i];
		}
	}
    ecef2pos(referencePosition, posr);
    ecef2enu(posr, vel+psol*3, enuv);

    drawGrid(c, positionToPoint(referencePosition), gint[scale], ng[scale], Color::LightGray, QColor(QColorConstants::LightGray));

    if (ui->btnPnt->isChecked()) {
        for (int i = 0; i < mainForm->NMapPnt + 1; i++) {
            drawPoint(c, i == 0 ? referencePosition : mainForm->PntPos[i-1] , ui->cBPntList->itemText(i),
                      i == pntIndex ? Qt::gray : QColor(QColorConstants::LightGray));
		}
	}
    if (ui->btnTrack->isChecked()) {
        QPoint *p =new QPoint[nsol];
        QColor *col = new QColor[nsol];
        int n = 0;
        for (int i = psols; i != psole;) {
            p[n] = positionToPoint(sol+i*3);
            col[n++] = color[stat[i]];
            if (++i >= nsol) i = 0;
		}
        c.setPen(Color::LightGray);
        c.drawPolyline(p, n-1);
        for (int i = 0; i < n; i++) {
            drawCircle(c,p[i],HISSIZE,col[i],col[i]);
		}
		delete [] p;
		delete [] col;
	}
    p0 = positionToPoint(currentPosition);
    c.setPen(Qt::black);
    c.drawLine(p0.x() - SOLSIZE / 2 - 4, p0.y(), p0.x() + SOLSIZE / 2 + 4, p0.y());
    c.drawLine(p0.x(), p0.y() - SOLSIZE / 2 - 4, p0.x(), p0.y() + SOLSIZE / 2 + 4);
    drawCircle(c, p0, SOLSIZE + 4, Qt::black, Qt::white);
    drawCircle(c, p0, SOLSIZE, Qt::black, color[currentstat]);

    drawVelocity(c, enuv);
    drawScale(c);
	
    c.setBrush(Qt::white);
    drawText(c, 6, 1, solstr[0], currentstat ? colsol:Qt::gray, 0);
    QRect off = c.boundingRect(QRect(), 0, solstr[2]);
    drawText(c, canvas.width() - off.width() - 6, 1, solstr[2], Qt::gray, 0);
    drawText(c, 50, 1, solstr[1], Qt::black, 0);

    if (ui->btnPnt->isChecked()) {
        pntpos = pntIndex > 0 ? mainForm->PntPos[pntIndex-1] : referencePosition;
        for (int i = 0; i < 3; i++) {
            rr[i]=pntpos[i]-currentPosition[i];
		}
        ecef2pos(pntpos, pos);
        ecef2enu(posr, rr, enu);
        dir = norm(enu, 2) < 1E-9 ? 0.0 : atan2(enu[0], enu[1])*R2D;
        if (dir < 0.0) dir += 360.0;
        s = tr("To %1: Distance %2m Direction %3%4")
                .arg(ui->cBPntList->itemText(pntIndex)).arg(norm(enu, 2), 0, 'f', 3).arg(dir, 0, 'f', 1).arg(CHARDEG);
        int y = canvas.height() - off.height() / 2 - 2;
        if (ui->btnGraph->isChecked()) y -= GRPHEIGHT + 2;
        drawText(c, canvas.width() / 2, y, s, Qt::gray, 1);
	}
    if (ui->btnGraph->isChecked()) {
        drawVerticalGraph(c, sol, stat, psol, psols, psole, nsol, currentstat);
	}
}
//---------------------------------------------------------------------------
void MapDialog::drawVerticalGraph(QPainter &c, const double *sol,
	const int *stat, int psol, int psols, int psole, int nsol, int currentstat)
{
    QColor color[] = {Qt::white, Qt::green, Color::Orange, Color::Fuchsia, Qt::blue, Qt::red};
    QRect rg(GRPHMARGIN,canvas.height() - GRPHEIGHT - 2, canvas.width() - GRPHMARGIN, canvas.height() - 2);
    QPoint *p = new QPoint[nsol], p0;
    QColor *col = new QColor[nsol];
    int n = 0, m = 0;
	
    for (int i = psols; i != psole;) {
        n++;
        if (++i >= nsol) i = 0;
	}
    for (int i = psols; i != psole;) {
        p[m] = positionToGraphP(sol + i * 3, sol + psol * 3, nsol - n + m, nsol, rg);
        if (i == psol) p0 = p[m];
        col[m++] = color[stat[i]];
        if (++i >= nsol) i = 0;
	}
    c.setBrush(Qt::white);
    c.fillRect(rg, QBrush(Qt::white));
    c.setPen(QColor(QColorConstants::LightGray));
    c.drawRect(rg);
    c.drawLine(rg.left(), (rg.top() + rg.bottom()) / 2,
               rg.right(), (rg.top() + rg.bottom()) / 2);
    c.setPen(QColor(QColorConstants::LightGray));
    c.drawLine(p0.x(), rg.bottom(), p0.x(), rg.top());
    for (int i = 0; i < nsol; i++) {
        if (i % 100) continue;
        int x = rg.left() + (rg.right() - rg.left()) * ((nsol + i - psole) % nsol) / nsol;
        int y = (rg.top() + rg.bottom()) / 2;
        c.drawLine(x, y - HISSIZE, x, y + HISSIZE);
	}
    int i, j;
    for (i = 0; i < m; i = j) {
        for (j = i; j < m; j++) {
            if (p[j].y() < rg.top() + HISSIZE / 2 || rg.bottom() - HISSIZE / 2 < p[j].y()) break;
		}
        if (i < j) c.drawPolyline(p + i, j - i - 1);
        for (;j < m; j++) {
            if (rg.top() + HISSIZE / 2 <= p[j].y() && p[j].y() <= rg.bottom() - HISSIZE / 2) break;
		}
	}
    for (int i = 0; i < m; i++) {
        if (p[i].y() < rg.top() + HISSIZE / 2 || rg.bottom() - HISSIZE / 2 < p[i].y()) continue;
        drawCircle(c, p[i], HISSIZE, col[i], col[i]);
	}
    drawCircle(c, p0, HISSIZE + 6, Qt::black, color[currentstat]);
	delete [] p;
	delete [] col;
}
//---------------------------------------------------------------------------
QPoint MapDialog::positionToPoint(const double *pos)
{
    double sc[] = {
        0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000
	};
    double rr[3], posr[3], enu[3], fact = 40.0 / sc[scale];
    QPoint p;
    ecef2pos(referencePosition, posr);
    for (int i = 0; i < 3; i++) rr[i] = pos[i] - referencePosition[i];
    ecef2enu(posr, rr, enu);
    p.setX(canvas.width() / 2 + (int)(enu[0] * fact + 0.5));
    p.setY(canvas.height() / 2 - (int)(enu[1] * fact + 0.5));
    if (ui->btnCenter->isChecked()) {
        for (int i = 0; i < 3; i++) rr[i] = currentPosition[i] - referencePosition[i];
        ecef2enu(posr, rr, enu);
        p.rx() -= (int)(enu[0] * fact + 0.5);
        p.ry() += (int)(enu[1] * fact + 0.5);
	}
    else if (norm(centerPosition, 3) > 0.0) {
        for (int i = 0; i < 3; i++) rr[i] = centerPosition[i] - referencePosition[i];
        ecef2enu(posr, rr, enu);
        p.rx() -= (int)(enu[0] * fact + 0.5);
        p.ry() += (int)(enu[1] * fact + 0.5);
	}
	return p;
}
//---------------------------------------------------------------------------
QPoint MapDialog::positionToGraphP(const double *pos,
    const double *ref, int index, int npos, QRect rect)
{
    double sc[] = {
        0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000
	};
    double rr[3], posr[3], enu[3], fact = 40.0 / sc[scale];
    QPoint p;
    ecef2pos(ref, posr);
    for (int i = 0; i < 3; i++) rr[i] = pos[i] - ref[i];
    ecef2enu(posr, rr, enu);
    p.setX(rect.left() + (int)((rect.right() - rect.left()) * index / (npos - 1.0) + 0.5));
    p.setY((rect.top() + rect.bottom()) / 2 - (int)(enu[2] * fact + 0.5));
	return p;
}
//---------------------------------------------------------------------------
void MapDialog::drawPoint(QPainter &c, const double *pos, QString name,
    QColor color)
{
    QPoint p = positionToPoint(pos);
    QRect off = c.boundingRect(QRect(), 0, name);
    drawCircle(c, p, PNTSIZE, color, color);
    c.setPen(color);
    c.drawLine(p.x() - PNTSIZE / 2 - 4, p.y(), p.x() + PNTSIZE / 2 + 4, p.y());
    c.drawLine(p.x(), p.y() - PNTSIZE / 2 - 4, p.x(), p.y() + PNTSIZE / 2 + 4);
    drawText(c,p.x(), p.y() + PNTSIZE / 2 + off.height() / 2 + 1, name, color, 1);
}
//---------------------------------------------------------------------------
void MapDialog::drawVelocity(QPainter &c, const double *vel)
{
    double v, ang;
    QPoint p;
    QColor color = Qt::gray;
	
    p.setX(VELSIZE / 2 + 10);
    p.setY(canvas.height() - VELSIZE / 2 - 15);
    c.setBrush(Qt::NoBrush);
    if ((v = norm(vel, 3)) > 1.0) {
        ang = atan2(vel[0], vel[1]) * R2D;
        DrawArrow(c, p, VELSIZE - 2, (int)ang, color);
	}
    drawCircle(c, p, VELSIZE, color, (QColor) - 1);
    drawCircle(c, p, 9, color, Qt::white);
    drawText(c, p.x(), p.y() - VELSIZE / 2 - 7, "N", color, 1);
    drawText(c, p.x(), p.y() + VELSIZE / 2 + 7, QString("%1 km/h").arg(v*3.6, 0, 'f', 0), color, 1);
}
//---------------------------------------------------------------------------
void  MapDialog::drawScale(QPainter &c)
{
    double sc[] = {
        0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000
	};
    int pc[][2] = {{-20, -4}, {-20, 4}, {-20, 0}, {20, 0}, {20, -4}, {20, 4}};
    int x = canvas.width() - 35, y = canvas.height() - 15;
    QPoint p[6];
	
    for (int i = 0; i < 6; i++) {
        p[i].setX(x + pc[i][0]);
        p[i].setY(y + pc[i][1]);
	}
    c.setPen(Qt::gray);
    c.drawPolyline(p, 5);
    c.setBrush(Qt::NoBrush);
    QString unit = "m";
    double sf = sc[scale];
    if (sf < 1.0) {sf *= 100.0; unit = "cm";}
    else if (sf >= 1000.0) {sf /= 1000.0; unit = "km";}
    drawText(c, x, y - 10, QString("%1 %2").arg(sf, 0, 'f', 0).arg(unit), Qt::gray, 1);
}
//---------------------------------------------------------------------------
void MapDialog::drawCircle(QPainter &c, QPoint p, int r, QColor color1,
    QColor color2)
{
    int x1 = p.x() - r / 2, x2 = r, y1 = p.y() - r / 2, y2 = r;
    c.setPen(color1);
    if (color2 != -1) {
        c.setBrush(color2);
    } else {
        c.setBrush(Qt::NoBrush);
	}
    c.drawEllipse(x1, y1, x2, y2);
}
//---------------------------------------------------------------------------
void MapDialog::drawGrid(QPainter &c, QPoint p, int gint, int ng,
    QColor color1, QColor color2)
{
    int i, w = canvas.width(), h = canvas.height();
    for (i = -p.x() / gint; i <= (w - p.x()) / gint; i++) {
        c.setPen(i % ng ? color1 : color2);
        c.drawLine(p.x() + i * gint, 0, p.x() + i * gint, h);
	}
    for (i = -p.y() / gint; i <= (h - p.y()) / gint; i++) {
        c.setPen(i % ng ? color1 : color2);
        c.drawLine(0, p.y() + i * gint, w, p.y() + i * gint);
	}
}
//---------------------------------------------------------------------------
void MapDialog::drawText(QPainter &c,int x, int y, QString s,
    QColor color, int align)
{
    QRect off=c.boundingRect(QRect(),0,s);
    if (align==1) {x-=off.width()/2; y-=off.height()/2;} else if (align==2) x-=off.width();
    c.setPen(color);
    c.drawText(x,y,s);
}
//---------------------------------------------------------------------------
void  MapDialog::DrawArrow(QPainter &c,QPoint p, int siz, int ang,
    QColor color)
{
    QPoint p1[6], p2[6];
    p1[0].rx() = p1[1].rx() = 0; p1[2].rx() = 3; p1[3].rx() = 0; p1[4].rx() = -3; p1[5].rx() = 0;
    p1[0].ry() = -siz / 2; p1[1].ry() = siz / 2; p1[2].ry() = p1[4].ry() = p1[1].y() - 6;
    p1[3].ry() = p1[5].ry() = siz / 2;
	
    for (int i = 0; i < 6; i++) {
        p2[i].setX(p.x() + (int)(p1[i].x() * cos(-ang * D2R) - p1[i].y() * sin(-ang * D2R) + 0.5));
        p2[i].setY(p.y() - (int)(p1[i].x() * sin(-ang * D2R) + p1[i].y() * cos(-ang * D2R) + 0.5));
	}
    c.setPen(color);
    c.drawPolyline(p2, 5);
}
//---------------------------------------------------------------------------
void  MapDialog::updatePntList(void)
{
    ui->cBPntList->clear();
    ui->cBPntList->addItem(referenceName);
    for (int i = 0; i < mainForm->NMapPnt; i++) {
        ui->cBPntList->addItem(mainForm->PntName[i]);
	}
    if (pntIndex >= ui->cBPntList->count()) pntIndex--;
    ui->cBPntList->setCurrentIndex(pntIndex);
}
//---------------------------------------------------------------------------
void  MapDialog::updateEnable(void)
{
    ui->cBPntList->setEnabled(ui->btnPnt->isChecked());
    ui->btnPntDlg->setEnabled(ui->btnPnt->isChecked());
}
//---------------------------------------------------------------------------
