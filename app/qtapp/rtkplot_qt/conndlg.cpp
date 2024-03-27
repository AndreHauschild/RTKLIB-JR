//---------------------------------------------------------------------------
#include <QShowEvent>

#include "rtklib.h"
#include "serioptdlg.h"
#include "fileoptdlg.h"
#include "tcpoptdlg.h"
#include "cmdoptdlg.h"
#include "conndlg.h"

//---------------------------------------------------------------------------

ConnectDialog::ConnectDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    stream1 = stream2 = format1 = format2 = 0;
    commandEnable1[0] = commandEnable1[1] = commandEnable2[0] = commandEnable2[1] = 0;

    connect(btnCancel, SIGNAL(clicked(bool)), this, SLOT(reject()));
    connect(btnOk, SIGNAL(clicked(bool)), this, SLOT(btnOkClick()));
    connect(btnCommand1, SIGNAL(clicked(bool)), this, SLOT(btnCommand1Clicked()));
    connect(btnCommand2, SIGNAL(clicked(bool)), this, SLOT(btnCommand2Clicked()));
    connect(btnOption1, SIGNAL(clicked(bool)), this, SLOT(btnOption1Clicked()));
    connect(btnOption2, SIGNAL(clicked(bool)), this, SLOT(btnOption2Clicked()));
    connect(cBSolutionFormat1, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEnable()));
    connect(cBSolutionFormat2, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEnable()));
    connect(cBSelectStream1, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEnable()));
    connect(cBSelectStream2, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEnable()));
}
//---------------------------------------------------------------------------
void ConnectDialog::showEvent(QShowEvent *event)
{
    int str[] = { STR_NONE, STR_SERIAL, STR_TCPCLI, STR_TCPSVR, STR_NTRIPCLI, STR_FILE };

    if (event->spontaneous()) return;

    for (int i = 0; i < 6; i++) {
        if (str[i] == stream1) cBSelectStream1->setCurrentIndex(i);
        if (str[i] == stream2) cBSelectStream2->setCurrentIndex(i);
	}
    cBSolutionFormat1->setCurrentIndex(format1);
    cBSolutionFormat2->setCurrentIndex(format2);
    cBTimeFormat->setCurrentIndex(timeFormat);
    cBDegFormat->setCurrentIndex(degFormat);
    lEFieldSeperator->setText(fieldSeparator);
    sBTimeoutTime->setValue(timeoutTime);
    sBReconnectTime->setValue(reconnectTime);

	updateEnable();
}
//---------------------------------------------------------------------------
void ConnectDialog::btnOkClick()
{
    int str[] = { STR_NONE, STR_SERIAL, STR_TCPCLI, STR_TCPSVR, STR_NTRIPCLI, STR_FILE };

    stream1 = str[cBSelectStream1->currentIndex()];
    stream2 = str[cBSelectStream2->currentIndex()];
    format1 = cBSolutionFormat1->currentIndex();
    format2 = cBSolutionFormat2->currentIndex();
    timeFormat = cBTimeFormat->currentIndex();
    degFormat = cBDegFormat->currentIndex();
    fieldSeparator = lEFieldSeperator->text();
    timeoutTime = sBTimeoutTime->value();
    reconnectTime = sBReconnectTime->value();

    accept();
}
//---------------------------------------------------------------------------
void ConnectDialog::btnOption1Clicked()
{
    switch (cBSelectStream1->currentIndex()) {
        case 1: serialOption1(0); break;
        case 2: tcpOption1(1);   break;
        case 3: tcpOption1(0);   break;
        case 4: tcpOption1(3);   break;
        case 5: fileOption1(0);   break;
	}
}
//---------------------------------------------------------------------------
void ConnectDialog::btnOption2Clicked()
{
    switch (cBSelectStream2->currentIndex()) {
        case 1: serialOption2(0); break;
        case 2: tcpOption2(1);   break;
        case 3: tcpOption2(0);   break;
        case 4: tcpOption2(3);   break;
        case 5: fileOption2(0);   break;
	}
}
//---------------------------------------------------------------------------
void ConnectDialog::btnCommand1Clicked()
{
    CmdOptDialog dialog(this);

    dialog.commands  [0] = commands1  [0];
    dialog.commands  [1] = commands1  [1];
    dialog.commandsEnabled[0] = commandEnable1[0];
    dialog.commandsEnabled[1] = commandEnable1[1];
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    commands1  [0] = dialog.commands  [0];
    commands1  [1] = dialog.commands  [1];
    commandEnable1[0] = dialog.commandsEnabled[0];
    commandEnable1[1] = dialog.commandsEnabled[1];
}
//---------------------------------------------------------------------------
void ConnectDialog::btnCommand2Clicked()
{
    CmdOptDialog dialog(this);

    dialog.commands  [0] = commands2  [0];
    dialog.commands  [1] = commands2  [1];
    dialog.commandsEnabled[0] = commandEnable2[0];
    dialog.commandsEnabled[1] = commandEnable2[1];
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    commands2  [0] = dialog.commands  [0];
    commands2  [1] = dialog.commands  [1];
    commandEnable2[0] = dialog.commandsEnabled[0];
    commandEnable2[1] = dialog.commandsEnabled[1];
}
//---------------------------------------------------------------------------
void ConnectDialog::serialOption1(int opt)
{
    SerialOptDialog dialog(this);

    dialog.path = paths1[0];
    dialog.options = opt;
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    paths1[0] = dialog.path;
}
//---------------------------------------------------------------------------
void ConnectDialog::serialOption2(int opt)
{
    SerialOptDialog dialog(this);

    dialog.path = paths2[0];
    dialog.options = opt;
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    paths2[0] = dialog.path;
}
//---------------------------------------------------------------------------
void ConnectDialog::tcpOption1(int opt)
{
    TcpOptDialog dialog(this);

    dialog.path = paths1[1];
    dialog.showOptions = opt;
    for (int i = 0; i < MAXHIST; i++) dialog.history [i] = TcpHistory [i];
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    paths1[1] = dialog.path;
    for (int i = 0; i < MAXHIST; i++) TcpHistory [i] = dialog.history [i];
}
//---------------------------------------------------------------------------
void ConnectDialog::tcpOption2(int opt)
{
    TcpOptDialog dialog(this);

    dialog.path = paths2[1];
    dialog.showOptions = opt;
    for (int i = 0; i < MAXHIST; i++) dialog.history [i] = TcpHistory [i];
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    paths2[1] = dialog.path;
    for (int i = 0; i < MAXHIST; i++) TcpHistory [i] = dialog.history [i];
}
//---------------------------------------------------------------------------
void ConnectDialog::fileOption1(int opt)
{
    FileOptDialog dialog(this);

    dialog.path = paths1[2];
    dialog.options = opt;
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    paths1[2] = dialog.path;
}
//---------------------------------------------------------------------------
void ConnectDialog::fileOption2(int opt)
{
    FileOptDialog dialog(this);

    dialog.path = paths2[2];
    dialog.options = opt;
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;

    paths2[2] = dialog.path;
}
//---------------------------------------------------------------------------
void ConnectDialog::updateEnable(void)
{
    btnOption1->setEnabled(cBSelectStream1->currentIndex() > 0);
    btnOption2->setEnabled(cBSelectStream2->currentIndex() > 0);
    btnCommand1->setEnabled(cBSelectStream1->currentIndex() == 1);
    btnCommand2->setEnabled(cBSelectStream2->currentIndex() == 1);
    cBSolutionFormat1->setEnabled(cBSelectStream1->currentIndex() > 0);
    cBSolutionFormat2->setEnabled(cBSelectStream2->currentIndex() > 0);
    cBTimeFormat->setEnabled(cBSolutionFormat1->currentIndex() != 3 || cBSolutionFormat2->currentIndex() != 3);
    cBDegFormat->setEnabled(cBSolutionFormat1->currentIndex() == 0 || cBSolutionFormat2->currentIndex() == 0);
    lEFieldSeperator->setEnabled(cBSolutionFormat1->currentIndex() != 3 || cBSolutionFormat2->currentIndex() != 3);
    Label5->setEnabled(cBSolutionFormat1->currentIndex() != 3 || cBSolutionFormat2->currentIndex() != 3);
    Label6->setEnabled(cBSolutionFormat1->currentIndex() == 0 || cBSolutionFormat2->currentIndex() == 0);
    Label7->setEnabled(cBSolutionFormat1->currentIndex() != 3 || cBSolutionFormat2->currentIndex() != 3);
    Label8->setEnabled((2 <= cBSelectStream1->currentIndex() && cBSelectStream1->currentIndex() <= 4) ||
                       (2 <= cBSelectStream2->currentIndex() && cBSelectStream2->currentIndex() <= 4));
    sBTimeoutTime->setEnabled((2 <= cBSelectStream1->currentIndex() && cBSelectStream1->currentIndex() <= 4) ||
                              (2 <= cBSelectStream2->currentIndex() && cBSelectStream2->currentIndex() <= 4));
    sBReconnectTime->setEnabled((2 <= cBSelectStream1->currentIndex() && cBSelectStream1->currentIndex() <= 4) ||
                                (2 <= cBSelectStream2->currentIndex() && cBSelectStream2->currentIndex() <= 4));
}
//---------------------------------------------------------------------------
