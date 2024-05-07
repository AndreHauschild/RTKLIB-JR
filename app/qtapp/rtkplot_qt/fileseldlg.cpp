//---------------------------------------------------------------------------

#include "plotmain.h"
#include "fileseldlg.h"

#include <QShowEvent>
#include <QDir>
#include <QFileInfoList>
#include <QTreeView>
#include <QFileSystemModel>
#include <QRegularExpression>
#include <QPushButton>

#include "ui_fileseldlg.h"


//---------------------------------------------------------------------------
FileSelDialog::FileSelDialog(Plot *plt, QWidget *parent)
    : QDialog(parent), plot(plt), ui(new Ui::FileSelDialog)
{
    ui->setupUi(this);

    dirModel = new QFileSystemModel(this);
    dirModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

#ifdef FLOATING_DIRSELECTOR
    directorySelector = new QTreeView(0);
    directorySelector->setWindowFlags(Qt::Window | Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint);
#else
    directorySelector = new QTreeView(this);
#endif
    ui->vPanelDirectory->layout()->addWidget(directorySelector);
    directorySelector->setModel(dirModel);
    directorySelector->hideColumn(1); directorySelector->hideColumn(2); directorySelector->hideColumn(3); //only show names

    fileModel = new QFileSystemModel(this);
    fileModel->setFilter((fileModel->filter() & ~QDir::Dirs & ~QDir::AllDirs));
    fileModel->setNameFilterDisables(false);
    ui->lVFileList->setModel(fileModel);

    connect(ui->cBDriveSelect, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FileSelDialog::driveSelectionChanged);
    connect(directorySelector, &QTreeView::clicked, this, &FileSelDialog::directroySelectChanged);
    connect(directorySelector, &QTreeView::doubleClicked, this, &FileSelDialog::hideDirectorySelector);
    connect(ui->btnDirectorySelect, &QPushButton::clicked, this, &FileSelDialog::toggleDirectorySelectorVisibility);
    connect(ui->lVFileList, &QListView::clicked, this, &FileSelDialog::fileListClicked);
    connect(ui->cBFilter, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FileSelDialog::updateFilter);
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
        ui->cBDriveSelect->setVisible(true);
        ui->cBDriveSelect->clear();

        foreach(const QFileInfo &drive, drives)
            ui->cBDriveSelect->addItem(drive.filePath());
    } else {
        ui->cBDriveSelect->setVisible(false); // do not show drive selection on unix
    }
}
//---------------------------------------------------------------------------
void FileSelDialog::driveSelectionChanged()
{
    directorySelector->setVisible(false);

    directorySelector->setRootIndex(dirModel->index(ui->cBDriveSelect->currentText()));
    ui->lblDirectorySelected->setText(ui->cBDriveSelect->currentText());
}
//---------------------------------------------------------------------------
void FileSelDialog::toggleDirectorySelectorVisibility()
{
#ifdef FLOATING_DIRSELECTOR
    QPoint pos = Panel5->mapToGlobal(btnDirectorySelect->pos());
    pos.rx() += btnDirectorySelect->width() - directorySelector->width();
    pos.ry() += btnDirectorySelect->height();

    DirSelector->move(pos);
#endif
    directorySelector->setVisible(!directorySelector->isVisible());
}
//---------------------------------------------------------------------------
void FileSelDialog::directroySelectChanged(QModelIndex index)
{
    directorySelector->expand(index);

    QString directory = dirModel->filePath(index);
    ui->lblDirectorySelected->setText(directory);
    fileModel->setRootPath(directory);
    ui->lVFileList->setRootIndex(fileModel->index(directory));
}
//---------------------------------------------------------------------------
void FileSelDialog::hideDirectorySelector(QModelIndex)
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
void FileSelDialog::updateFilter()
{
    QString filter = ui->cBFilter->currentText();

    // only keep data between brackets
    filter = filter.mid(filter.indexOf("(") + 1);
    filter.remove(")");

    fileModel->setNameFilters(filter.split(" ", Qt::SkipEmptyParts));
    directorySelector->setVisible(false);
}
//---------------------------------------------------------------------------
QString FileSelDialog::getDirectory()
{
    return fileModel->rootPath();
}
//---------------------------------------------------------------------------
void FileSelDialog::setDirectory(const QString &dir)
{
    QFileInfoList drives = QDir::drives();
    QString directory = dir;

    if (directory.isEmpty()) directory = drives.at(0).filePath();

    ui->cBDriveSelect->setCurrentText(directory.mid(0, directory.indexOf(":") + 2));
    dirModel->setRootPath(directory);
    directorySelector->setVisible(false);
    ui->lblDirectorySelected->setText(directory);
    fileModel->setRootPath(directory);
    ui->lVFileList->setRootIndex(fileModel->index(directory));

}
//---------------------------------------------------------------------------
