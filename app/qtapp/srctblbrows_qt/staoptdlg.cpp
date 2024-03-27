//---------------------------------------------------------------------------
#include <QShowEvent>
#include <QFileDialog>

#include "browsmain.h"
#include "staoptdlg.h"


extern MainForm *mainForm;

//---------------------------------------------------------------------------
StaListDialog::StaListDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    connect(btnCancel, &QPushButton::clicked, this, &StaListDialog::reject);
    connect(btnLoad, &QPushButton::clicked, this, &StaListDialog::btnLoadClicked);
    connect(btnOk, &QPushButton::clicked, this, &StaListDialog::btnOkClicked);
    connect(btnSave, &QPushButton::clicked, this, &StaListDialog::btnSaveClicked);
}
//---------------------------------------------------------------------------
void StaListDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    lWStationList->clear();

    for (int i = 0; i < mainForm->stationList.count(); i++)
        lWStationList->addItem(mainForm->stationList.at(i));
}
//---------------------------------------------------------------------------
void StaListDialog::btnOkClicked()
{
    mainForm->stationList.clear();

    for (int i = 0; i < lWStationList->count(); i++)
        mainForm->stationList.append(lWStationList->item(i)->text());
}
//---------------------------------------------------------------------------
void StaListDialog::btnLoadClicked()
{
    QString filename;
    QFile file;
    QByteArray buffer;

    filename = QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open...")));

    file.setFileName(filename);
    if (!file.open(QIODevice::ReadOnly)) return;

    lWStationList->clear();
    lWStationList->setVisible(false);

    while (!file.atEnd()) {
        buffer = file.readLine();
        buffer = buffer.mid(buffer.indexOf('#'));
        lWStationList->addItem(buffer);
    }

    lWStationList->setVisible(true);
}
//---------------------------------------------------------------------------
void StaListDialog::btnSaveClicked()
{
    QString filename = QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Save...")));
    QFile file;

    file.setFileName(filename);
    if (!file.open(QIODevice::WriteOnly)) return;

    for (int i = 0; i < lWStationList->count(); i++)
        file.write((lWStationList->item(i)->text() + "\n").toLatin1());
}
//---------------------------------------------------------------------------
