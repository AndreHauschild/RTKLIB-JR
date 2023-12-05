//---------------------------------------------------------------------------
#include <QShortcutEvent>
#include <QCompleter>
#include <QFileSystemModel>
#include <QFileDialog>

#include "rtklib.h"
#include "serioptdlg.h"
#include "fileoptdlg.h"
#include "tcpoptdlg.h"
#include "outstrdlg.h"
#include "keydlg.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
OutputStrDialog::OutputStrDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    keyDialog = new KeyDialog(this);
    serialOptDialog = new SerialOptDialog(this);
    tcpOptDialog = new TcpOptDialog(this);

    QCompleter *fileCompleter = new QCompleter(this);
    QFileSystemModel *fileModel = new QFileSystemModel(fileCompleter);
    fileModel->setRootPath("");
    fileCompleter->setModel(fileModel);
    lEFilePath1->setCompleter(fileCompleter);
    lEFilePath2->setCompleter(fileCompleter);

    connect(btnCancel, &QPushButton::clicked, this, &OutputStrDialog::reject);
    connect(btnFile1, &QPushButton::clicked, this, &OutputStrDialog::btnFile1Clicked);
    connect(btnFile2, &QPushButton::clicked, this, &OutputStrDialog::btnFile2Clicked);
    connect(btnKey, &QPushButton::clicked, this, &OutputStrDialog::btnKeyClicked);
    connect(btnOk, &QPushButton::clicked, this, &OutputStrDialog::btnOkClicked);
    connect(btnStream1, &QPushButton::clicked, this, &OutputStrDialog::btnStream1Clicked);
    connect(btnStream2, &QPushButton::clicked, this, &OutputStrDialog::btnStream2Clicked);
    connect(cBStream1, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OutputStrDialog::updateEnable);
    connect(cBStream2, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OutputStrDialog::updateEnable);
    connect(cBStream1C, &QCheckBox::clicked, this, &OutputStrDialog::updateEnable);
    connect(cBStream2C, &QCheckBox::clicked, this, &OutputStrDialog::updateEnable);

    cBSwapInterval->setValidator(new QDoubleValidator(this));
}
//---------------------------------------------------------------------------
void OutputStrDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;
    
    cBStream1C->setChecked(streamEnabled[0]);
    cBStream2C->setChecked(streamEnabled[1]);
    cBStream1->setCurrentIndex(stream[0]);
    cBStream2->setCurrentIndex(stream[1]);
    cBFormat1->setCurrentIndex(format[0]);
    cBFormat2->setCurrentIndex(format[1]);
    lEFilePath1->setText(getFilePath(paths[0][2]));
    lEFilePath2->setText(getFilePath(paths[1][2]));
    cBSwapInterval->insertItem(0, swapInterval); cBSwapInterval->setCurrentIndex(0);
    cBTimeTag->setChecked(outputTimeTag);

    updateEnable();
}
//---------------------------------------------------------------------------
void OutputStrDialog::btnOkClicked()
{
    streamEnabled[0] = cBStream1C->isChecked();
    streamEnabled[1] = cBStream2C->isChecked();
    stream[0] = cBStream1->currentIndex();
    stream[1] = cBStream2->currentIndex();
    format[0] = cBFormat1->currentIndex();
    format[1] = cBFormat2->currentIndex();
    paths [0][2] = setFilePath(lEFilePath1->text());
    paths [1][2] = setFilePath(lEFilePath2->text());
    swapInterval = cBSwapInterval->currentText();
    outputTimeTag = cBTimeTag->isChecked();

    accept();
}
//---------------------------------------------------------------------------
void OutputStrDialog::btnFile1Clicked()
{
    lEFilePath1->setText(QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Load..."), lEFilePath1->text())));
}
//---------------------------------------------------------------------------
void OutputStrDialog::btnFile2Clicked()
{
    lEFilePath2->setText(QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Load..."), lEFilePath2->text())));
}
//---------------------------------------------------------------------------
void OutputStrDialog::btnKeyClicked()
{
    keyDialog->exec();
}
//---------------------------------------------------------------------------
void OutputStrDialog::btnStream1Clicked()
{
    switch (cBStream1->currentIndex()) {
        case 0: serialOptions(0, 0); break;
        case 1: tcpOptions(0, 1); break;
        case 2: tcpOptions(0, 0); break;
        case 3: tcpOptions(0, 2); break;
        case 4: tcpOptions(0, 4); break;
    }
}
//---------------------------------------------------------------------------
void OutputStrDialog::btnStream2Clicked()
{
    switch (cBStream2->currentIndex()) {
        case 0: serialOptions(1, 0); break;
        case 1: tcpOptions(1, 1); break;
        case 2: tcpOptions(1, 0); break;
        case 3: tcpOptions(1, 2); break;
        case 4: tcpOptions(0, 4); break;
    }
}
//---------------------------------------------------------------------------
QString OutputStrDialog::getFilePath(const QString path)
{
    return path.mid(0, path.indexOf("::"));
}
//---------------------------------------------------------------------------
QString OutputStrDialog::setFilePath(const QString p)
{
    QString path = p;
    QString str;
    bool okay;

    if (cBTimeTag->isChecked()) path += "::T";
    str = cBSwapInterval->currentText();
    str.toDouble(&okay);
    if (okay)
        path += "::S=" + str;
    return path;
}
//---------------------------------------------------------------------------
void OutputStrDialog::serialOptions(int index, int opt)
{
    serialOptDialog->path = paths[index][0];
    serialOptDialog->options = opt;

    serialOptDialog->exec();
    if (serialOptDialog->result() != QDialog::Accepted) return;

    paths[index][0] = serialOptDialog->path;
}
//---------------------------------------------------------------------------
void OutputStrDialog::tcpOptions(int index, int opt)
{
    tcpOptDialog->path = paths[index][1];
    tcpOptDialog->showOptions = opt;
    for (int i = 0; i < 10; i++) {
        tcpOptDialog->history[i] = history[i];
	}

    tcpOptDialog->exec();
    if (tcpOptDialog->exec() != QDialog::Accepted) return;

    paths[index][1] = tcpOptDialog->path;
    for (int i = 0; i < 10; i++) {
        history[i] = tcpOptDialog->history[i];
	}
}
//---------------------------------------------------------------------------
void OutputStrDialog::updateEnable(void)
{
    int ena = (cBStream1C->isChecked() && (cBStream1->currentIndex() == 5)) ||
              (cBStream2C->isChecked() && (cBStream2->currentIndex() == 5));

    cBStream1->setEnabled(cBStream1C->isChecked());
    cBStream2->setEnabled(cBStream2C->isChecked());
    btnStream1->setEnabled(cBStream1C->isChecked() && cBStream1->currentIndex() <= 4);
    btnStream2->setEnabled(cBStream2C->isChecked() && cBStream2->currentIndex() <= 4);
    lEFilePath1->setEnabled(cBStream1C->isChecked() && cBStream1->currentIndex() == 5);
    lEFilePath2->setEnabled(cBStream2C->isChecked() && cBStream2->currentIndex() == 5);
    btnFile1->setEnabled(cBStream1C->isChecked() && cBStream1->currentIndex() == 5);
    btnFile2->setEnabled(cBStream2C->isChecked() && cBStream2->currentIndex() == 5);
    lblF1->setEnabled(ena);
    lblSwapInterval->setEnabled(ena);
    lblH->setEnabled(ena);
    cBTimeTag->setEnabled(ena);
    cBSwapInterval->setEnabled(ena);
    btnKey->setEnabled(ena);
}
//---------------------------------------------------------------------------
