//---------------------------------------------------------------------------
#ifndef mapdlgH
#define mapdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class MapDialog;
}

class PntDialog;

//---------------------------------------------------------------------------
class MapDialog : public QDialog
{
    Q_OBJECT

protected:
    void  showEvent(QShowEvent*);
    void  mousePressEvent(QMouseEvent *);
    void  mouseReleaseEvent(QMouseEvent *);
    void  mouseMoveEvent(QMouseEvent *);

public slots:
    void  FormResize();
    void  DispPaint();
    void  shrinkMap();
    void  expandMap();
    void  btnPntDlgClick();
    void  centerMap();
    void  btnTrackClick();
    void  btnPntClick();
    void  pntListChange();
private:

    QPixmap canvas;
    QString referenceName;
    double centerPositionDrag0[3];
    int scale, pntIndex, dragging, dragX0, dragY0;
    PntDialog *pntDialog;
    Ui::MapDialog *ui;

    void drawVerticalGraph(QPainter &c, const double *sol,
		const int *stat, int psol, int psols, int psole, int nsol, int currentstat);
    QPoint positionToPoint(const double *pos);
    QPoint positionToGraphP(const double *pos, const double *ref,
        int index, int npos, QRect rect);
    void drawPoint(QPainter &c, const double *pos, QString name, QColor color);
    void drawVelocity(QPainter &c, const double *vel);
    void drawScale(QPainter &c);
    void drawCircle(QPainter &c, QPoint p, int r, QColor color1, QColor color2);
    void drawGrid(QPainter &c, QPoint p, int gint, int ng, QColor color1, QColor color2);
    void drawText(QPainter &c, int x, int y, QString s, QColor color, int align);
    void DrawArrow(QPainter &c, QPoint p, int siz, int ang, QColor color);
    void updatePntList();
    void updateEnable();

public:
    double currentPosition[3], referencePosition[3], centerPosition[3];

    void resetReference(void);
    void updateMap(const double *sol, const double *solref,
		const double *vel, const int *stat, int psol, int psols, int psole,
        int nsol, QString *solstr, int currentstat);
     MapDialog(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
