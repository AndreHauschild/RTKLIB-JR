//---------------------------------------------------------------------------

#ifndef mapviewoptH
#define mapviewoptH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class MapViewOpt;
}

//---------------------------------------------------------------------------
class MapViewOptDialog : public QDialog
{
    Q_OBJECT

public:
    MapViewOptDialog(QWidget* parent);

    void setApiKey(const QString &);
    QString getApiKey();

    void setMapStrings(int, const QString &, const QString &, const QString &);
    void getMapStrings(int, QString &, QString &, QString &);

private:
    Ui::MapViewOpt *ui;

protected:
    QString attrs[6];

public slots:
    void showNotes();    
};

//---------------------------------------------------------------------------
#endif
