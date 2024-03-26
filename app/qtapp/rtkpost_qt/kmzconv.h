//---------------------------------------------------------------------------

#ifndef kmzconvH
#define kmzconvH
//---------------------------------------------------------------------------
#include <QDialog>
#include <QSettings>

namespace Ui {
class ConvDialog;
}

class TextViewer;
//---------------------------------------------------------------------------
class ConvDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConvDialog(QWidget *parent);

    void setInput(const QString &File);

    void loadOptions(QSettings&);
    void saveOptions(QSettings&);

protected slots:
    void convert();
    void viewOutputFile();
    void selectInputFile();
    void selectGoogleEarthFile();
    void callGoogleEarth();
    void formatChanged();
    void updateEnable();

private:
    int execCommand(const QString &cmd, const QStringList &opt);
    void showMessage(const QString &msg);
    void updateOutputFile();

    Ui::ConvDialog *ui;

protected:
    TextViewer *viewer;

};
//---------------------------------------------------------------------------
#endif
