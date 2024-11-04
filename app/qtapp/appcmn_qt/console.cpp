//---------------------------------------------------------------------------
#include <ctype.h>
#include <stdio.h>

#include <QScrollBar>

#include "console.h"
#include "ui_console.h"

#define MAXLEN		256
#define MAXLINE		2048

//---------------------------------------------------------------------------
Console::Console(QWidget *parent)
    : QDialog(parent), ui(new Ui::Console)
{
    ui->setupUi(this);

    consoleBuffer.reserve(MAXLINE);
    consoleBuffer.append("");

    ui->btnHex->setChecked(true);
    ui->btnAsc->setChecked(false);

    connect(ui->btnClose, &QPushButton::clicked, this, &Console::close);
    connect(ui->btnClear, &QPushButton::clicked, this, &Console::btnClearClicked);
    connect(ui->btnAsc, &QPushButton::clicked, this, &Console::btnAsciiClicked);
    connect(ui->btnDown, &QPushButton::clicked, this, &Console::btnDownClicked);
    connect(ui->btnHex, &QPushButton::clicked, this, &Console::btnHexClicked);
}
//---------------------------------------------------------------------------
void Console::btnAsciiClicked()
{
    ui->btnHex->setChecked(!ui->btnAsc->isChecked());
}
//---------------------------------------------------------------------------
void Console::btnHexClicked()
{
    ui->btnAsc->setChecked(!ui->btnHex->isChecked());
}
//---------------------------------------------------------------------------
void Console::btnClearClicked()
{
    consoleBuffer.clear();
    consoleBuffer.reserve(MAXLINE);
    consoleBuffer.append("");
    ui->textEdit->setPlainText("");
}
//---------------------------------------------------------------------------
void Console::btnDownClicked()
{
    ui->textEdit->verticalScrollBar()->setValue(ui->textEdit->verticalScrollBar()->maximum());
}
//---------------------------------------------------------------------------
void Console::addMessage(uint8_t *msg, int n)
{
    char buff[MAXLEN+16], *p = buff;
    int mode = ui->btnAsc->isChecked();

    if (n <= 0) return;

    if (ui->btnStop->isChecked()) return;

    p += sprintf(p, "%s", qPrintable(consoleBuffer.last()));

    for (int i = 0; i < n; i++) {
            if (mode) {
                    if (msg[i] == '\r') continue;
                    p += sprintf(p, "%c", msg[i] == '\n' || isprint(msg[i]) ? msg[i] : '.');
            }
            else {
                    p += sprintf(p, "%s%02X", (p - buff) % 17 == 16 ? " " : "", msg[i]);
                    if (p - buff >= 67) p += sprintf(p,"\n");
            }
            if (p-buff >= MAXLEN) p += sprintf(p,"\n");

            if (*(p-1) == '\n') {
                    consoleBuffer.last() = buff;
                    consoleBuffer.append("");
                    *(p=buff) = 0;
                    if (consoleBuffer.count() >= MAXLINE) consoleBuffer.removeFirst();
            }
    }
    consoleBuffer.last() = buff;

    ui->textEdit->setPlainText(consoleBuffer.join(QString()));
}
//---------------------------------------------------------------------------
