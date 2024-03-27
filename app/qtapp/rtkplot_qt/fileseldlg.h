//---------------------------------------------------------------------------
#ifndef fileseldlgH
#define fileseldlgH

//---------------------------------------------------------------------------
#include <QDialog>

#include "ui_fileseldlg.h"

class QShowEVent;
class QTreeView;
class QFileSystemModel;
class QModelIndex;

//---------------------------------------------------------------------------
class FileSelDialog : public QDialog, private Ui::FileSelDialog
{
    Q_OBJECT

public slots:
    void fileListClicked(QModelIndex);
    void directroySelectChanged(QModelIndex);
    void directorySelectSelected(QModelIndex);
    void driveSelectionChanged();
    void filterClicked();
    void btnDirectorySelectClicked();

protected:
    void showEvent(QShowEvent*);
    QTreeView *directorySelector;
    QFileSystemModel *fileModel, *dirModel;

public:
    QString directory;

    explicit FileSelDialog(QWidget *parent=0);
    ~FileSelDialog();
};
//---------------------------------------------------------------------------
#endif
