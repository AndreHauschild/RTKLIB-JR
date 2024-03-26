//---------------------------------------------------------------------------

#ifndef markdlgH
#define markdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class MarkDialog;
}

class KeyDialog;

//---------------------------------------------------------------------------
class MarkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MarkDialog(QWidget *parent);

    void setPositionMode(int);
    int getPositionMode();

    void setName(const QString&);
    QString getName();

    void setComment(const QString&);
    QString getComment();

    void setStationPositionFile(const QString&);
    QString getStationPositionFile();

public slots:
    void saveClose();
    void showKeyDialog();
    void btnPositionClicked();

protected:
    int nMark;
    QString stationPositionFileF;
    int positionMode;

private:
    void updateEnable();
    KeyDialog *keyDialog;

    Ui::MarkDialog *ui;
	
};
//---------------------------------------------------------------------------
#endif
