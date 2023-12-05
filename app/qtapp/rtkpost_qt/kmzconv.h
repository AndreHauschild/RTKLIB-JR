//---------------------------------------------------------------------------

#ifndef kmzconvH
#define kmzconvH
//---------------------------------------------------------------------------
#include <QDialog>
#include "ui_kmzconv.h"

class TextViewer;
//---------------------------------------------------------------------------
class ConvDialog : public QDialog, public Ui::ConvDialog
{
    Q_OBJECT

public slots:
    void btnConvertClicked();
    void btnViewClicked();
    void btnInputFileClicked();
    void googleEarthFileChanged();
    void btnGoogleEarthFileClicked();
    void btnGoogleEarthClick();
    void formatKMLClicked();
    void formatGPXClicked();
    void updateEnable();

private:
    int execCommand(const QString &cmd, const QStringList &opt);
    void showMessage(const QString &msg);
    void updateOutputFile();

protected:
    void showEvent(QShowEvent*);

    TextViewer *viewer;

public:
    explicit ConvDialog(QWidget *parent);

    void setInput(const QString &File);
};
//---------------------------------------------------------------------------
#endif
