//---------------------------------------------------------------------------
#ifndef viewerH
#define viewerH
//---------------------------------------------------------------------------
#define MAXLINE		100000

#include <QDialog>
#include "ui_viewer.h"

class ViewerOptDialog;

//---------------------------------------------------------------------------
class TextViewer : public QDialog, private Ui::TextViewer
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent*);

    ViewerOptDialog *viewerOptDialog;

public slots:
    void btnReadClicked();
    void btnOptionsClicked();
    void btnReloadClicked();
    void btnFindClicked();

private:
    QString file;
	
    void updateText(void);

public:
    int option;  // 0: disable file loading; 1(default): allow file loading; 2: switch to file saving
    static QColor colorText, colorBackground;
    static QFont font;

    bool read(const QString &file);
    bool save(const QString &file);

    explicit TextViewer(QWidget* parent);
};
//---------------------------------------------------------------------------
#endif
