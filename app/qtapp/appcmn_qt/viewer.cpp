//---------------------------------------------------------------------------
// ported to Qt by Jens Reimann
#include <QShowEvent>
#include <QFileDialog>
#include <QPalette>
#include <QDebug>

#include "rtklib.h"
#include "viewer.h"
#include "vieweropt.h"

QColor TextViewer::colorText = Qt::black, TextViewer::colorBackground = Qt::black;
QFont TextViewer::font;

//---------------------------------------------------------------------------
TextViewer::TextViewer(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    option = 1;

    viewerOptDialog = new ViewerOptDialog(this);

    connect(btnClose, &QPushButton::clicked, this, &TextViewer::accept);
    connect(btnFind, &QPushButton::clicked, this, &TextViewer::btnFindClicked);
    connect(btnOpt, &QPushButton::clicked, this, &TextViewer::btnOptionsClicked);
    connect(btnRead, &QPushButton::clicked, this, &TextViewer::btnReadClicked);
    connect(btnReload, &QPushButton::clicked, this, &TextViewer::btnReloadClicked);
    connect(findStr, &QLineEdit::editingFinished, this, &TextViewer::btnFindClicked);
}
//---------------------------------------------------------------------------
void TextViewer::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    if (option == 0) {
        btnReload->setVisible(false);
        btnRead->setVisible(false);
    } else if (option == 2) {
        btnReload->setVisible(false);
        btnRead->setText(tr("Save..."));
	}

	updateText();
}
//---------------------------------------------------------------------------
void TextViewer::btnReloadClicked()
{
	read(file);
}
//---------------------------------------------------------------------------
void TextViewer::btnReadClicked()
{
    if (btnRead->text() == tr("Save..."))
        save(QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, QString(), file)));
    else
        read(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, QString(), file)));
}
//---------------------------------------------------------------------------
void TextViewer::btnOptionsClicked()
{
    viewerOptDialog->font = font;
    viewerOptDialog->colorText = colorText;
    viewerOptDialog->colorBackground = colorBackground;

    viewerOptDialog->move(this->size().width() / 2 - viewerOptDialog->size().width() / 2,
                          this->size().height() / 2 - viewerOptDialog->size().height() / 2);
    viewerOptDialog->exec();

    if (viewerOptDialog->result() != QDialog::Accepted) return;

    font = viewerOptDialog->font;
    colorText = viewerOptDialog->colorText;
    colorBackground = viewerOptDialog->colorBackground;

	updateText();
}
//---------------------------------------------------------------------------
void TextViewer::btnFindClicked()
{
    textEdit->find(findStr->text());
}
//---------------------------------------------------------------------------
bool TextViewer::read(const QString &path)
{
    char filename[1024], *p[] = { filename };

    if (expath(qPrintable(path), p, 1) < 1) return false;

    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly)) return false;
    textEdit->setPlainText("");

    QString TextStr = f.readAll();
    textEdit->appendPlainText(TextStr);

    setWindowTitle(filename);
    file = filename;

    return true;
}
//---------------------------------------------------------------------------
bool TextViewer::save(const QString &filename)
{
    QFile f(filename);

    if (!f.open(QIODevice::WriteOnly)) return false;

    f.write(textEdit->toPlainText().toLocal8Bit());
    file = filename;

    return true;
}
//---------------------------------------------------------------------------
void TextViewer::updateText(void)
{
    QPalette pal;

    textEdit->setFont(font);
    pal = textEdit->palette();
    pal.setColor(QPalette::Text, colorText);
    pal.setColor(QPalette::Base, colorBackground);
    textEdit->setPalette(pal);
}
//---------------------------------------------------------------------------
