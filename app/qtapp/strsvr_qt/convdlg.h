//---------------------------------------------------------------------------
#ifndef convdlgH
#define convdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class ConvDialog;
}

//---------------------------------------------------------------------------
class ConvDialog : public QDialog
{
    Q_OBJECT

public slots:
    void updateEnable();

private:
    Ui::ConvDialog *ui;

public:
    explicit ConvDialog(QWidget *parent);

    void setConversionEnabled(bool enable);
    bool getConversionEnabled();
    void setInputFormat(int format);
    int getInputFormat();
    void setOutputFormat(int format);
    int getOutputFormat();
    void setConversionMessage(const QString &);
    QString getConversionMessage();
    void setConversionOptions(const QString &);
    QString getConversionOptions();
};
//---------------------------------------------------------------------------
#endif
