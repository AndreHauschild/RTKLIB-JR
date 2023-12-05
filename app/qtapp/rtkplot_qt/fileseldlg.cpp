//---------------------------------------------------------------------------

#include "plotmain.h"
#include "fileseldlg.h"

#include <QShowEvent>
#include <QDir>
#include <QFileInfoList>
#include <QTreeView>
#include <QFileSystemModel>
#include <QRegularExpression>

extern Plot *plot;

//---------------------------------------------------------------------------
FileSelDialog::FileSelDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    dirModel = new QFileSystemModel(this);
    dirModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

#ifdef FLOATING_DIRSELECTOR
    DirSelector = new QTreeView(0);
    DirSelector->setWindowFlags(Qt::Window | Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint);
#else
    directorySelector = new QTreeView(this);
#endif
    Panel2->layout()->addWidget(directorySelector);
    directorySelector->setModel(dirModel);
    directorySelector->hideColumn(1); directorySelector->hideColumn(2); directorySelector->hideColumn(3); //only show names

    fileModel = new QFileSystemModel(this);
    fileModel->setFilter((fileModel->filter() & ~QDir::Dirs & ~QDir::AllDirs));
    fileModel->setNameFilterDisables(false);
    lVFileList->setModel(fileModel);

    connect(cBDriveSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(driveSelectionChanged()));
    connect(directorySelector, SIGNAL(clicked(QModelIndex)), this, SLOT(directroySelectChanged(QModelIndex)));
    connect(directorySelector, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(directorySelectSelected(QModelIndex)));
    connect(btnDirectorySelect, SIGNAL(clicked(bool)), this, SLOT(btnDirectorySelectClicked()));
    connect(lVFileList, SIGNAL(clicked(QModelIndex)), this, SLOT(fileListClicked(QModelIndex)));
    connect(cBFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(filterClicked()));
}
//---------------------------------------------------------------------------
FileSelDialog::~FileSelDialog()
{
    delete directorySelector;
}
//---------------------------------------------------------------------------
void FileSelDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    QFileInfoList drives = QDir::drives();
    if (drives.size() > 1 && drives.at(0).filePath() != "/") {
        Panel1->setVisible(true);
        cBDriveSelect->clear();

        foreach(const QFileInfo &drive, drives) {
            cBDriveSelect->addItem(drive.filePath());
        }
    } else {
        Panel1->setVisible(false); // do not show drive selection on unix
    }
    if (directory == "") directory = drives.at(0).filePath();

    cBDriveSelect->setCurrentText(directory.mid(0, directory.indexOf(":") + 2));
    dirModel->setRootPath(directory);
    directorySelector->setVisible(false);
    lblDirectorySelected->setText(directory);
    fileModel->setRootPath(directory);
    lVFileList->setRootIndex(fileModel->index(directory));
}
//---------------------------------------------------------------------------
void FileSelDialog::driveSelectionChanged()
{
    directorySelector->setVisible(false);

    directorySelector->setRootIndex(dirModel->index(cBDriveSelect->currentText()));
    lblDirectorySelected->setText(cBDriveSelect->currentText());
}
//---------------------------------------------------------------------------
void FileSelDialog::btnDirectorySelectClicked()
{
#ifdef FLOATING_DIRSELECTOR
    QPoint pos = Panel5->mapToGlobal(BtnDirSel->pos());
    pos.rx() += BtnDirSel->width() - DirSelector->width();
    pos.ry() += BtnDirSel->height();

    DirSelector->move(pos);
#endif
    directorySelector->setVisible(!directorySelector->isVisible());
}
//---------------------------------------------------------------------------
void FileSelDialog::directroySelectChanged(QModelIndex index)
{
    directorySelector->expand(index);

    directory = dirModel->filePath(index);
    lblDirectorySelected->setText(directory);
    fileModel->setRootPath(directory);
    lVFileList->setRootIndex(fileModel->index(directory));
}
//---------------------------------------------------------------------------
void FileSelDialog::directorySelectSelected(QModelIndex)
{
    directorySelector->setVisible(false);
}
//---------------------------------------------------------------------------
void FileSelDialog::fileListClicked(QModelIndex index)
{
    QStringList file;

    file.append(fileModel->filePath(index));
    plot->readSolution(file, 0);

    directorySelector->setVisible(false);
}
//---------------------------------------------------------------------------
void FileSelDialog::filterClicked()
{
    QString filter = cBFilter->currentText();

    // only keep data between brackets
    filter = filter.mid(filter.indexOf("(") + 1);
    filter.remove(")");

    fileModel->setNameFilters(filter.split(" "));
    directorySelector->setVisible(false);
}
//---------------------------------------------------------------------------
