//---------------------------------------------------------------------------
#ifndef fileseldlgH
#define fileseldlgH

//---------------------------------------------------------------------------
#include <QDialog>
#include <QModelIndex>

class QShowEVent;
class QTreeView;
class QFileSystemModel;
class Plot;

namespace Ui {
class FileSelDialog;
}

//---------------------------------------------------------------------------
class FileSelDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileSelDialog(Plot *plot, QWidget *parent=0);
    ~FileSelDialog();

    QString getDirectory();
    void setDirectory(const QString &);

public slots:
    void fileListClicked(QModelIndex);
    void directroySelectChanged(QModelIndex);
    void hideDirectorySelector(QModelIndex);
    void driveSelectionChanged();
    void updateFilter();
    void toggleDirectorySelectorVisibility();

protected:
    void showEvent(QShowEvent*);
    QTreeView *directorySelector;
    QFileSystemModel *fileModel, *dirModel;
    Plot *plot;

private:
    Ui::FileSelDialog *ui;
};
//---------------------------------------------------------------------------
#endif
