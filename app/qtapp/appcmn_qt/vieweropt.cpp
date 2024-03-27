//---------------------------------------------------------------------------
// ported to Qt by Jens Reimann

#include <QDialog>
#include <QShowEvent>
#include <QFont>
#include <QPalette>
#include <QColorDialog>
#include <QFontDialog>

#include "helper.h"
#include "viewer.h"
#include "vieweropt.h"

//---------------------------------------------------------------------------
ViewerOptDialog::ViewerOptDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    connect(btnCancel, &QPushButton::clicked, this, &ViewerOptDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &ViewerOptDialog::accept);
    connect(btnFont, &QPushButton::clicked, this, &ViewerOptDialog::btnFontClicked);
    connect(btnColorText, &QPushButton::clicked, this, &ViewerOptDialog::btnColorTextClicked);
    connect(btnColorBackground, &QPushButton::clicked, this, &ViewerOptDialog::btnColorBackgroundClicked);
}
//---------------------------------------------------------------------------
void ViewerOptDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    fontLabel->setFont(font);
    fontLabel->setText(QString("%1 %2 px").arg(font.family()).arg(font.pointSize()));

    lbColorText->setStyleSheet(QString("background-color: %1").arg(color2String(colorText)));
    lbColorBackground->setStyleSheet(QString("background-color: %1").arg(color2String(colorBackground)));
}
//---------------------------------------------------------------------------
void ViewerOptDialog::btnColorTextClicked()
{
    QColorDialog d;

    d.setCurrentColor(colorText);
    d.exec();
    colorText = d.selectedColor();

    lbColorText->setStyleSheet(QString("background-color: %1").arg(color2String(colorText)));
}
//---------------------------------------------------------------------------
void ViewerOptDialog::btnColorBackgroundClicked()
{
    QColorDialog d;

    d.setCurrentColor(colorBackground);
    d.exec();
    colorBackground = d.selectedColor();

    lbColorBackground->setStyleSheet(QString("background-color: %1").arg(color2String(colorBackground)));
}
//---------------------------------------------------------------------------
void ViewerOptDialog::btnFontClicked()
{
    QFontDialog d;

    d.setCurrentFont(font);
    d.exec();

    font = d.selectedFont();

    fontLabel->setFont(font);
    fontLabel->setText(font.family() + QString::number(font.pointSize()) + "pt");
}
//---------------------------------------------------------------------------
