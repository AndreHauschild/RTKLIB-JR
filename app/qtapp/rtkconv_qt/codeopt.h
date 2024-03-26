//---------------------------------------------------------------------------
#ifndef codeoptH
#define codeoptH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class CodeOptDialog;
}

//---------------------------------------------------------------------------
class CodeOptDialog : public QDialog
{
    Q_OBJECT

public:
    CodeOptDialog(QWidget *parent);

    void setNavSystem(int navSystem);
    int getNavSystem() {return navSystem;}
    void setFrequencyType(int frequencyType);
    int getFrequencyType() {return frequencyType;}
    void setCodeMask(int i, const QString& mask);
    QString getCodeMask(int i);

protected slots:
    void setUnsetAll();

protected:
    int navSystem, frequencyType;
    void updateEnable();

private:
    Ui::CodeOptDialog *ui;

};
//----------------------------------------------------------------------
#endif
