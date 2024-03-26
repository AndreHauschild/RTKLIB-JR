//---------------------------------------------------------------------------

#ifndef vieweroptH
#define vieweroptH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class ViewerOptDialog;
}

//---------------------------------------------------------------------------
class ViewerOptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ViewerOptDialog(QWidget* parent);

    QFont getFont() {return font;}
    void setFont(const QFont &font);
    const QColor& getTextColor() {return colorText;}
    void setTextColor(const QColor &);
    const QColor& getBackgroundColor() {return colorBackground;}
    void setBackgroundColor(const QColor &);

public slots:
    void selectTextColor();
    void selectBackgroundColor();
    void selectFont();

protected:
    QFont font;
    QColor colorText, colorBackground;

private:
    Ui::ViewerOptDialog *ui;
};
//---------------------------------------------------------------------------
#endif
