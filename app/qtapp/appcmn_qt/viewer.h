//---------------------------------------------------------------------------
#ifndef viewerH
#define viewerH
//---------------------------------------------------------------------------
#define MAXLINE		100000

#include <QDialog>
#include <QSettings>

namespace Ui {
class TextViewer;
}

class ViewerOptDialog;

//---------------------------------------------------------------------------
class TextViewer : public QDialog
{
    Q_OBJECT

public:
    explicit TextViewer(QWidget* parent, int option = 1);

    bool read(const QString &file);
    bool save(const QString &file);

    void loadOptions(QSettings &);
    void saveOptions(QSettings &);

    void setOption(int option);  // 0: disable file loading; 1(default): allow file loading; 2: switch to file saving

protected:
    void showEvent(QShowEvent*);

    ViewerOptDialog *viewerOptDialog;
    QColor colorText, colorBackground;
    QFont font;

public slots:
    void readSaveFile();
    void showOptions();
    void reloadText();
    void findText();

private:
    QString file;
    Ui::TextViewer *ui;
	
    void updateText();
};
//---------------------------------------------------------------------------
#endif
