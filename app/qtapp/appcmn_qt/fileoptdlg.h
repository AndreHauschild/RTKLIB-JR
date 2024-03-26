//---------------------------------------------------------------------------
#ifndef fileoptdlgH
#define fileoptdlgH
//---------------------------------------------------------------------------
#include <QDialog>

namespace Ui {
class FileOptDialog;
}

class KeyDialog;

//---------------------------------------------------------------------------
class FileOptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileOptDialog(QWidget* parent, int options = 0, int pathEnabled = 0);

    void setPath(const QString &path);
    QString getPath();
    void setOptions(int options);  // 0: input file; 1: output file
    int getOptions() {return options;}
    void setPathEnabled(int);
    int getPathEnabled() {return pathEnabled;}

private:
    Ui::FileOptDialog *ui;

protected:
    KeyDialog *keyDialog;

    int options, pathEnabled;

protected slots:
    void filePathSelect();
    void keyDialogShow();
    void updateEnable();
};
//---------------------------------------------------------------------------
#endif
