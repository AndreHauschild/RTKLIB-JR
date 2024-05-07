//---------------------------------------------------------------------------
#include <QShortcutEvent>
#include <QCompleter>
#include <QFileSystemModel>
#include <QFileDialog>
#include <QAction>

#include "serioptdlg.h"
#include "fileoptdlg.h"
#include "tcpoptdlg.h"
#include "outstrdlg.h"
#include "keydlg.h"

#include "ui_outstrdlg.h"


//---------------------------------------------------------------------------
OutputStrDialog::OutputStrDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::OutputStrDialog)
{
    ui->setupUi(this);

    keyDialog = new KeyDialog(this);
    serialOptDialog = new SerialOptDialog(this);
    tcpOptDialog = new TcpOptDialog(this);

    QCompleter *fileCompleter = new QCompleter(this);
    QFileSystemModel *fileModel = new QFileSystemModel(fileCompleter);
    fileModel->setRootPath("");
    fileCompleter->setModel(fileModel);
    ui->lEFilePath1->setCompleter(fileCompleter);
    ui->lEFilePath2->setCompleter(fileCompleter);

    // line edit actions
    QAction *aclEFilePath1Select = ui->lEFilePath1->addAction(QIcon(":/buttons/folder"), QLineEdit::TrailingPosition);
    aclEFilePath1Select->setToolTip(tr("Select File"));
    QAction *aclEFilePath2Select = ui->lEFilePath2->addAction(QIcon(":/buttons/folder"), QLineEdit::TrailingPosition);
    aclEFilePath2Select->setToolTip(tr("Select File"));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &OutputStrDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &OutputStrDialog::reject);
    connect(aclEFilePath1Select, &QAction::triggered, this, &OutputStrDialog::selectFile1);
    connect(aclEFilePath2Select, &QAction::triggered, this, &OutputStrDialog::selectFile2);
    connect(ui->btnKey, &QPushButton::clicked, this, &OutputStrDialog::showKeyDialog);
    connect(ui->btnStream1, &QPushButton::clicked, this, &OutputStrDialog::showStream1Options);
    connect(ui->btnStream2, &QPushButton::clicked, this, &OutputStrDialog::showStream2Options);
    connect(ui->cBStream1, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OutputStrDialog::updateEnable);
    connect(ui->cBStream2, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OutputStrDialog::updateEnable);
    connect(ui->cBStream1C, &QCheckBox::clicked, this, &OutputStrDialog::updateEnable);
    connect(ui->cBStream2C, &QCheckBox::clicked, this, &OutputStrDialog::updateEnable);

    ui->cBSwapInterval->setValidator(new QDoubleValidator(this));
}
//---------------------------------------------------------------------------
void OutputStrDialog::selectFile1()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Load..."), ui->lEFilePath1->text());

    if (!filename.isEmpty())
        ui->lEFilePath1->setText(QDir::toNativeSeparators(filename));
}
//---------------------------------------------------------------------------
void OutputStrDialog::selectFile2()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Load..."), ui->lEFilePath2->text());
    if (!filename.isEmpty())
        ui->lEFilePath2->setText(QDir::toNativeSeparators(filename));
}
//---------------------------------------------------------------------------
void OutputStrDialog::showKeyDialog()
{
    keyDialog->exec();
}
//---------------------------------------------------------------------------
void OutputStrDialog::showStream1Options()
{
    switch (ui->cBStream1->currentIndex()) {
        case 0: showSerialOptions(0, 0); break;
        case 1: showTcpOptions(0, TcpOptDialog::OPT_TCP_CLIENT); break;
        case 2: showTcpOptions(0, TcpOptDialog::OPT_TCP_SERVER); break;
        case 3: showTcpOptions(0, TcpOptDialog::OPT_NTRIP_SERVER); break;
        case 4: showTcpOptions(0, TcpOptDialog::OPT_NTRIP_CASTER_CLIENT); break;
    }
}
//---------------------------------------------------------------------------
void OutputStrDialog::showStream2Options()
{
    switch (ui->cBStream2->currentIndex()) {
        case 0: showSerialOptions(1, 0); break;
        case 1: showTcpOptions(1, TcpOptDialog::OPT_TCP_CLIENT); break;
        case 2: showTcpOptions(1, TcpOptDialog::OPT_TCP_SERVER); break;
        case 3: showTcpOptions(1, TcpOptDialog::OPT_NTRIP_SERVER); break;
        case 4: showTcpOptions(0, TcpOptDialog::OPT_NTRIP_CASTER_CLIENT); break;
    }
}
//---------------------------------------------------------------------------
QString OutputStrDialog::getFilePath(const QString &path)
{
    return path.mid(0, path.indexOf("::"));
}
//---------------------------------------------------------------------------
QString OutputStrDialog::setFilePath(const QString &p)
{
    QString path = p;
    QString str;
    bool okay;

    if (ui->cBTimeTag->isChecked()) path += "::T";
    str = ui->cBSwapInterval->currentText();
    str.toDouble(&okay);
    if (okay)
        path += "::S=" + str;
    return path;
}
//---------------------------------------------------------------------------
void OutputStrDialog::showSerialOptions(int index, int opt)
{
    serialOptDialog->setPath(paths[index][0]);
    serialOptDialog->setOptions(opt);

    serialOptDialog->exec();
    if (serialOptDialog->result() != QDialog::Accepted) return;

    paths[index][0] = serialOptDialog->getPath();
}
//---------------------------------------------------------------------------
void OutputStrDialog::showTcpOptions(int index, int opt)
{
    tcpOptDialog->setPath(paths[index][1]);
    tcpOptDialog->setOptions(opt);
    tcpOptDialog->setHistory(history, 10);

    tcpOptDialog->exec();
    if (tcpOptDialog->exec() != QDialog::Accepted) return;

    paths[index][1] = tcpOptDialog->getPath();
    for (int i = 0; i < 10; i++) {
        history[i] = tcpOptDialog->getHistory()[i];
	}
}
//---------------------------------------------------------------------------
void OutputStrDialog::updateEnable()
{
    int ena = (ui->cBStream1C->isChecked() && (ui->cBStream1->currentIndex() == 5)) ||
              (ui->cBStream2C->isChecked() && (ui->cBStream2->currentIndex() == 5));

    ui->cBStream1->setEnabled(ui->cBStream1C->isChecked());
    ui->cBStream2->setEnabled(ui->cBStream2C->isChecked());
    ui->btnStream1->setEnabled(ui->cBStream1C->isChecked() && ui->cBStream1->currentIndex() <= 4);
    ui->btnStream2->setEnabled(ui->cBStream2C->isChecked() && ui->cBStream2->currentIndex() <= 4);
    ui->lEFilePath1->setEnabled(ui->cBStream1C->isChecked() && ui->cBStream1->currentIndex() == 5);
    ui->lEFilePath2->setEnabled(ui->cBStream2C->isChecked() && ui->cBStream2->currentIndex() == 5);
    ui->lblF1->setEnabled(ena);
    ui->lblSwapInterval->setEnabled(ena);
    ui->lblH->setEnabled(ena);
    ui->cBTimeTag->setEnabled(ena);
    ui->cBSwapInterval->setEnabled(ena);
    ui->btnKey->setEnabled(ena);
}
//---------------------------------------------------------------------------
void OutputStrDialog::setStreamEnabled(int stream, int enabled)
{
    QCheckBox *cBStreamC[] = {ui->cBStream1C, ui->cBStream2C};
    if (stream > 2) return;
    cBStreamC[stream]->setChecked(enabled);

    updateEnable();
}
//---------------------------------------------------------------------------
int OutputStrDialog::getStreamEnabled(int stream)
{
    QCheckBox *cBStreamC[] = {ui->cBStream1C, ui->cBStream2C};
    if (stream > 2) return -1;
    return cBStreamC[stream]->isChecked();
}
//---------------------------------------------------------------------------
void OutputStrDialog::setStreamType(int stream, int type)
{
    QComboBox *cBStream[] = {ui->cBStream1, ui->cBStream2};
    if (stream > 2) return;
    cBStream[stream]->setCurrentIndex(type);

    updateEnable();
}
//---------------------------------------------------------------------------
void OutputStrDialog::setStreamFormat(int stream, int format)
{
    QComboBox *cBFormat[] = {ui->cBFormat1, ui->cBFormat2};
    if (stream > 2) return;
    cBFormat[stream]->setCurrentIndex(format);
    updateEnable();
}
//---------------------------------------------------------------------------
int OutputStrDialog::getStreamFormat(int stream)
{
    QComboBox *cBFormat[] = {ui->cBFormat1, ui->cBFormat2};
    if (stream > 2) return -1;
    return cBFormat[stream]->currentIndex();
}

//---------------------------------------------------------------------------
int OutputStrDialog::getStreamType(int stream)
{
    QComboBox *cBStream[] = {ui->cBStream1, ui->cBStream2};
    if (stream > 2) return -1;
    return cBStream[stream]->currentIndex();
};
//---------------------------------------------------------------------------
void OutputStrDialog::setPath(int stream, int type, const QString &path)
{
    QLineEdit *edits[] = {ui->lEFilePath1, ui->lEFilePath2};
    if (stream > 2) return;
    paths[stream][type] = path;
    if (type == 2)
    {
        edits[stream]->setText(path);
    };
}
//---------------------------------------------------------------------------
QString OutputStrDialog::getPath(int stream, int type)
{
    QLineEdit *edits[] = {ui->lEFilePath1, ui->lEFilePath2};
    if (stream > 2) return "";
    if (type == 2)
        return setFilePath(edits[stream]->text());

    return paths[stream][type];
}
//---------------------------------------------------------------------------
void OutputStrDialog::setTimeTagEnabled(bool ena)
{
    ui->cBTimeTag->setChecked(ena);
}
//---------------------------------------------------------------------------
bool OutputStrDialog::getTimeTagEnabled(){
    return ui->cBTimeTag->isChecked();
}
//---------------------------------------------------------------------------
void OutputStrDialog::setSwapInterval(const QString & swapInterval)
{
    QString interval_str = swapInterval + " h";
    if (ui->cBSwapInterval->findText(interval_str) == -1)
        ui->cBSwapInterval->insertItem(0, interval_str);
    ui->cBSwapInterval->setCurrentText(interval_str);
}
//---------------------------------------------------------------------------
QString OutputStrDialog::getSwapInterval()
{
    return ui->cBSwapInterval->currentText().split(' ', Qt::SkipEmptyParts).first();
};
//---------------------------------------------------------------------------
void OutputStrDialog::setHistory(int i, const QString &history)
{
    this->history[i] = history;
}
//---------------------------------------------------------------------------
const QString &OutputStrDialog::getHistory(int i)
{
    return history[i];
}
//---------------------------------------------------------------------------
