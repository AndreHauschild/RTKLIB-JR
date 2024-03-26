//---------------------------------------------------------------------------
#include <QShowEvent>

#include "serioptdlg.h"
#include "fileoptdlg.h"
#include "tcpoptdlg.h"
#include "cmdoptdlg.h"
#include "conndlg.h"

#include "ui_conndlg.h"

#include "rtklib.h"


//---------------------------------------------------------------------------

ConnectDialog::ConnectDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ConnectDialog)
{
    ui->setupUi(this);

    for (int i = 0; i < 2; i++)
        for (int j  = 0; j < 2; j++)
            commandEnable[i][j] = 0;

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ConnectDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ConnectDialog::reject);
    connect(ui->btnCommand1, &QPushButton::clicked, this, &ConnectDialog::selectCommandsStream1);
    connect(ui->btnCommand2, &QPushButton::clicked, this, &ConnectDialog::selectCommandsStream2);
    connect(ui->btnOption1, &QPushButton::clicked, this, &ConnectDialog::selectOptionsStream1);
    connect(ui->btnOption2, &QPushButton::clicked, this, &ConnectDialog::selectOptionsStream2);
    connect(ui->cBSolutionFormat1, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ConnectDialog::updateEnable);
    connect(ui->cBSolutionFormat2, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ConnectDialog::updateEnable);
    connect(ui->cBSelectStream1, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ConnectDialog::updateEnable);
    connect(ui->cBSelectStream2, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ConnectDialog::updateEnable);
}
//---------------------------------------------------------------------------
void ConnectDialog::selectOptionsStream1()
{
    switch (ui->cBSelectStream1->currentIndex()) {
        case 1: serialOptionsStream(0, 0); break;
        case 2: tcpOption(0, TcpOptDialog::OPT_TCP_CLIENT);   break;
        case 3: tcpOption(0, TcpOptDialog::OPT_TCP_SERVER);   break;
        case 4: tcpOption(0, TcpOptDialog::OPT_NTRIP_CLIENT);   break;
        case 5: fileOption(0, 0);   break;
	}
}
//---------------------------------------------------------------------------
void ConnectDialog::selectOptionsStream2()
{
    switch (ui->cBSelectStream2->currentIndex()) {
        case 1: serialOptionsStream(1, 0); break;
        case 2: tcpOption(1, TcpOptDialog::OPT_TCP_CLIENT);   break;
        case 3: tcpOption(1, TcpOptDialog::OPT_TCP_SERVER);   break;
        case 4: tcpOption(1, TcpOptDialog::OPT_NTRIP_CLIENT);   break;
        case 5: fileOption(1, 0);   break;
	}
}
//---------------------------------------------------------------------------
void ConnectDialog::selectCommandsStream1()
{
    CmdOptDialog dialog(this);

    for (int i = 0; i < 2; i++) {
        dialog.setCommands(i, commands[0][i]);
        dialog.setCommandsEnabled(i, commandEnable[0][i]);
    }
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    for (int i = 0; i < 2; i++) {
        commands[0][i] = dialog.getCommands(i);
        commandEnable[0][i] = dialog.getCommandsEnabled(i);
    }
}
//---------------------------------------------------------------------------
void ConnectDialog::selectCommandsStream2()
{
    CmdOptDialog dialog(this);

    for (int i = 0; i < 2; i++) {
        dialog.setCommands(i, commands[1][i]);
        dialog.setCommandsEnabled(i, commandEnable[1][i]);
    }
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    for (int i = 0; i < 2; i++) {
        commands[1][i] = dialog.getCommands(i);
        commandEnable[1][i] = dialog.getCommandsEnabled(i);
    }
}
//---------------------------------------------------------------------------
void ConnectDialog::serialOptionsStream(int stream,int opt)
{
    SerialOptDialog dialog(this);

    dialog.setPath(paths[stream][0]);
    dialog.setOptions(opt);
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    paths[stream][0] = dialog.getPath();
}
//---------------------------------------------------------------------------
void ConnectDialog::tcpOption(int stream, int opt)
{
    TcpOptDialog dialog(this);

    dialog.setOptions(opt);
    dialog.setHistory(history, MAXHIST);
    dialog.setPath(paths[stream][1]);

    dialog.exec();
    if (dialog.result() != QDialog::Accepted) return;

    paths[stream][1] = dialog.getPath();
    for (int i = 0; i < MAXHIST; i++)
        history[i] = dialog.getHistory()[i];
}
//---------------------------------------------------------------------------
void ConnectDialog::fileOption(int stream, int opt)
{
    FileOptDialog dialog(this);

    dialog.setPath(paths[stream][2]);
    dialog.setOptions(opt);

    dialog.exec();
    if (dialog.result() != QDialog::Accepted) return;

    paths[stream][2] = dialog.getPath();
}
//---------------------------------------------------------------------------
void ConnectDialog::updateEnable()
{
    ui->btnOption1->setEnabled(ui->cBSelectStream1->currentIndex() > 0);
    ui->btnOption2->setEnabled(ui->cBSelectStream2->currentIndex() > 0);
    ui->btnCommand1->setEnabled(ui->cBSelectStream1->currentIndex() == 1);
    ui->btnCommand2->setEnabled(ui->cBSelectStream2->currentIndex() == 1);
    ui->cBSolutionFormat1->setEnabled(ui->cBSelectStream1->currentIndex() > 0);
    ui->cBSolutionFormat2->setEnabled(ui->cBSelectStream2->currentIndex() > 0);
    ui->cBTimeFormat->setEnabled(ui->cBSolutionFormat1->currentIndex() != 3 || ui->cBSolutionFormat2->currentIndex() != 3);
    ui->cBDegFormat->setEnabled(ui->cBSolutionFormat1->currentIndex() == 0 || ui->cBSolutionFormat2->currentIndex() == 0);
    ui->lEFieldSeperator->setEnabled(ui->cBSolutionFormat1->currentIndex() != 3 || ui->cBSolutionFormat2->currentIndex() != 3);
    ui->lblTimeFormat->setEnabled(ui->cBSolutionFormat1->currentIndex() != 3 || ui->cBSolutionFormat2->currentIndex() != 3);
    ui->lblLatLonFormat->setEnabled(ui->cBSolutionFormat1->currentIndex() == 0 || ui->cBSolutionFormat2->currentIndex() == 0);
    ui->lblFieldSeparator->setEnabled(ui->cBSolutionFormat1->currentIndex() != 3 || ui->cBSolutionFormat2->currentIndex() != 3);
    ui->lblTimeout->setEnabled((2 <= ui->cBSelectStream1->currentIndex() && ui->cBSelectStream1->currentIndex() <= 4) ||
                               (2 <= ui->cBSelectStream2->currentIndex() && ui->cBSelectStream2->currentIndex() <= 4));
    ui->lblReconnect->setEnabled((2 <= ui->cBSelectStream1->currentIndex() && ui->cBSelectStream1->currentIndex() <= 4) ||
                                 (2 <= ui->cBSelectStream2->currentIndex() && ui->cBSelectStream2->currentIndex() <= 4));
    ui->sBTimeoutTime->setEnabled((2 <= ui->cBSelectStream1->currentIndex() && ui->cBSelectStream1->currentIndex() <= 4) ||
                                  (2 <= ui->cBSelectStream2->currentIndex() && ui->cBSelectStream2->currentIndex() <= 4));
    ui->sBReconnectTime->setEnabled((2 <= ui->cBSelectStream1->currentIndex() && ui->cBSelectStream1->currentIndex() <= 4) ||
                                    (2 <= ui->cBSelectStream2->currentIndex() && ui->cBSelectStream2->currentIndex() <= 4));
}
//---------------------------------------------------------------------------
void ConnectDialog::setStreamType(int stream, int type)
{
    int str[] = {STR_NONE, STR_SERIAL, STR_TCPCLI, STR_TCPSVR, STR_NTRIPCLI, STR_FILE};
    QComboBox *cBType[] = {ui->cBSelectStream1, ui->cBSelectStream2};

    for (int i = 0; i < 6; i++) {
        if (str[i] == type) cBType[stream]->setCurrentIndex(i);
    }
    updateEnable();
}
//---------------------------------------------------------------------------
int ConnectDialog::getStreamType(int stream)
{
    int str[] = {STR_NONE, STR_SERIAL, STR_TCPCLI, STR_TCPSVR, STR_NTRIPCLI, STR_FILE};
    QComboBox *cBType[] = {ui->cBSelectStream1, ui->cBSelectStream2};

    return str[cBType[stream]->currentIndex()];
}
//---------------------------------------------------------------------------
void ConnectDialog::setStreamFormat(int stream, int format)
{
    QComboBox *cBFormat[] = {ui->cBSolutionFormat1, ui->cBSolutionFormat2};
    cBFormat[stream]->setCurrentIndex(format);

    updateEnable();
}
//---------------------------------------------------------------------------
int ConnectDialog::getStreamFormat(int stream)
{
    QComboBox *cBFormat[] = {ui->cBSolutionFormat1, ui->cBSolutionFormat2};
    return cBFormat[stream]->currentIndex();
}
//---------------------------------------------------------------------------
void ConnectDialog::setCommands(int stream, int type, const QString & cmd)
{
    commands[stream][type]= cmd;
}
//---------------------------------------------------------------------------
QString ConnectDialog::getCommands(int stream, int type)
{
    return commands[stream][type];
}
//---------------------------------------------------------------------------
void ConnectDialog::setCommandsEnabled(int stream, int type, bool ena)
{
    commandEnable[stream][type] = ena;
}
//---------------------------------------------------------------------------
bool ConnectDialog::getCommandsEnabled(int stream, int type)
{
    return commandEnable[stream][type];
}
//---------------------------------------------------------------------------
void ConnectDialog::setTimeFormat(int timeFormat)
{
    ui->cBTimeFormat->setCurrentIndex(timeFormat);
}
//---------------------------------------------------------------------------
int ConnectDialog::getTimeFormat()
{
    return ui->cBTimeFormat->currentIndex();
}
//---------------------------------------------------------------------------
void ConnectDialog::setDegFormat(int degFormat)
{
    ui->cBDegFormat->setCurrentIndex(degFormat);
}
//---------------------------------------------------------------------------
int ConnectDialog::getDegFormat()
{
    return ui->cBDegFormat->currentIndex();
}
//---------------------------------------------------------------------------
void ConnectDialog::setTimeoutTime(int timeoutTime)
{
    ui->sBTimeoutTime->setValue(timeoutTime);
}
//---------------------------------------------------------------------------
int ConnectDialog::getTimeoutTime()
{
    return ui->sBTimeoutTime->value();
}
//---------------------------------------------------------------------------
void ConnectDialog::setReconnectTime(int reconnectTime)
{
    ui->sBReconnectTime->setValue(reconnectTime);
}
//---------------------------------------------------------------------------
int ConnectDialog::getReconnectTime()
{
    return ui->sBReconnectTime->value();
}
//---------------------------------------------------------------------------
void ConnectDialog::setFieldSeparator(const QString &fieldSeparator)
{
    ui->lEFieldSeperator->setText(fieldSeparator);
}
//---------------------------------------------------------------------------
QString ConnectDialog::getFieldSeparator()
{
    return ui->lEFieldSeperator->text();
}
//---------------------------------------------------------------------------
void ConnectDialog::setPath(int stream, int type, const QString &path)
{
    paths[stream][type] = path;
}
//---------------------------------------------------------------------------
QString ConnectDialog::getPath(int stream, int type)
{
    return paths[stream][type];
}
//---------------------------------------------------------------------------
void ConnectDialog::setHistory(int i, const QString &history)
{
    if (i < MAXHIST)
        this->history[i] = history;
}
//---------------------------------------------------------------------------
const QString &ConnectDialog::getHistory(int i)
{
    return history[i];
}
//---------------------------------------------------------------------------
