//---------------------------------------------------------------------------
#ifndef cmdoptdlgH
#define cmdoptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class CmdOptDialog;
}

//---------------------------------------------------------------------------
class CmdOptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CmdOptDialog(QWidget* parent);

    void setCommands(int i, const QString &);
    QString getCommands(int i);
    void setCommandsEnabled(int i, bool);
    bool getCommandsEnabled(int i);

protected slots:
    void load();
    void save();
    void updateEnable();

private:
    Ui::CmdOptDialog * ui;
};
#endif
