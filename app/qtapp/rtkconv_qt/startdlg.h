//---------------------------------------------------------------------------
#ifndef startdlgH
#define startdlgH
//---------------------------------------------------------------------------
#include <QDialog>

#include "rtklib.h"

namespace Ui {
class StartDialog;
}

//---------------------------------------------------------------------------
class StartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartDialog(QWidget *parent = nullptr);

    gtime_t getTime();
    void setTime(const gtime_t&);
    const QString &getFileName() {return filename;}
    void setFileName(const QString &filename);

protected:
    QString filename;

private:
    Ui::StartDialog *ui;

protected slots:
    void setTimeFromFile();
};
//---------------------------------------------------------------------------
#endif
