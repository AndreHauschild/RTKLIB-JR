//---------------------------------------------------------------------------

#include "plotmain.h"
#include "plotopt.h"
#include "refdlg.h"
#include "viewer.h"
#include "rtklib.h"
#include "helper.h"

#include <QShowEvent>
#include <QColorDialog>
#include <QFontDialog>
#include <QFileDialog>
#include <QPoint>
#include <QDebug>
#include <QFileSystemModel>
#include <QCompleter>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
PlotOptDialog::PlotOptDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    refDialog = new RefDialog(this);

    QCompleter *fileCompleter = new QCompleter(this);
    QFileSystemModel *fileModel = new QFileSystemModel(fileCompleter);
    fileModel->setRootPath("");
    fileCompleter->setModel(fileModel);
    lETLEFile->setCompleter(fileCompleter);
    lETLESatelliteFile->setCompleter(fileCompleter);

    connect(btnCancel, SIGNAL(clicked(bool)), this, SLOT(reject()));
    connect(btnColor1, SIGNAL(clicked(bool)), this, SLOT(btnColor1Clicked()));
    connect(btnColor2, SIGNAL(clicked(bool)), this, SLOT(btnColor2Clicked()));
    connect(btnColor3, SIGNAL(clicked(bool)), this, SLOT(btnColor3Clicked()));
    connect(btnColor4, SIGNAL(clicked(bool)), this, SLOT(btnColor4Clicked()));
    connect(btnFont, SIGNAL(clicked(bool)), this, SLOT(btnFontClicked()));
    connect(btnOK, SIGNAL(clicked(bool)), this, SLOT(btnOKClicked()));
    connect(btnReferencePosition, SIGNAL(clicked(bool)), this, SLOT(btnReferencePositionClicked()));
    connect(btnTLEFile, SIGNAL(clicked(bool)), this, SLOT(btnTLEFileClicked()));
    connect(btnTLESatelliteFile, SIGNAL(clicked(bool)), this, SLOT(btnTLESatelliteFileClicked()));
    connect(btnTLESatelliteView, SIGNAL(clicked(bool)), this, SLOT(btnTLESatelliteViewClicked()));
    connect(btnTLEView, SIGNAL(clicked(bool)), this, SLOT(btnTLEViewClicked()));
    connect(btnShapeFile, SIGNAL(clicked(bool)), this, SLOT(btnShapeFileClicked()));
    connect(cBOrigin, SIGNAL(currentIndexChanged(int)), this, SLOT(originChanged()));
    connect(cBAutoScale, SIGNAL(currentIndexChanged(int)), this, SLOT(autoScaleChanged()));
    connect(cBReceiverPosition, SIGNAL(currentIndexChanged(int)), this, SLOT(receiverPositionChanged()));
    connect(lblMColor1, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor2, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor3, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor4, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor5, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor6, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor7, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor8, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor9, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor10, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor11, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(lblMColor12, SIGNAL(clicked(bool)), this, SLOT(mColorClicked()));
    connect(cBTimeSync, SIGNAL(clicked(bool)), this, SLOT(timeSyncClicked()));
}
//---------------------------------------------------------------------------
void PlotOptDialog::showEvent(QShowEvent *event)
{
    int marks[] = { 1, 2, 3, 4, 5, 10, 15, 20 };

    if (event->spontaneous()) return;

    cBTimeLabel->setCurrentIndex(plot->timeLabel);
    cBLatLonFormat->setCurrentIndex(plot->latLonFormat);
    cBAutoScale->setCurrentIndex(plot->autoScale);
    cBShowStats->setCurrentIndex(plot->showStats);
    cBShowArrow->setCurrentIndex(plot->showArrow);
    cBShowSlip->setCurrentIndex(plot->showSlip);
    cBShowHalfC->setCurrentIndex(plot->showHalfC);
    cBShowError->setCurrentIndex(plot->showError);
    cBShowEphemeris->setCurrentIndex(plot->showEphemeris);
    cBShowLabel->setCurrentIndex(plot->showLabel);
    cBShowGridLabel->setCurrentIndex(plot->showGridLabel);
    cBShowScale->setCurrentIndex(plot->showScale);
    cBShowCompass->setCurrentIndex(plot->showCompass);
    cBPlotStyle->setCurrentIndex(plot->plotStyle);

    for (int i = 0; i < 8; i++)
        if (marks[i] == plot->markSize) cBMarkSize->setCurrentIndex(i);

    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 2; j++)
            mColor[j][i] = plot->mColor[j][i];

    for (int i = 0; i < 4; i++)
        cColor[i] = plot->cColor[i];

    lblMColor1->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[0][1])));
    lblMColor2->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[0][2])));
    lblMColor3->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[0][3])));
    lblMColor4->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[0][4])));
    lblMColor5->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[0][5])));
    lblMColor6->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[0][6])));
    lblMColor7->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[1][1])));
    lblMColor8->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[1][2])));
    lblMColor9->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[1][3])));
    lblMColor10->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[1][4])));
    lblMColor11->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[1][5])));
    lblMColor12->setStyleSheet(QString("background-color: %1;").arg(color2String(plot->mColor[1][6])));
    lblColor1->setStyleSheet(QString("QLabel {background-color: %1;}").arg(color2String(plot->cColor[0])));
    lblColor2->setStyleSheet(QString("QLabel {background-color: %1;}").arg(color2String(plot->cColor[1])));
    lblColor3->setStyleSheet(QString("QLabel {background-color: %1;}").arg(color2String(plot->cColor[2])));
    lblColor4->setStyleSheet(QString("QLabel {background-color: %1;}").arg(color2String(plot->cColor[3])));

    //FontOpt = plot->Font;
    fontOption.setFamily(plot->fontName);
    fontOption.setPixelSize(plot->fontSize);
    updateFont();

    cBElevationMask->setCurrentText(QString::number(plot->elevationMask));
    cBMaxDop->setCurrentText(QString::number(plot->maxDop));
    cBMaxMP->setCurrentText(QString::number(plot->maxMP));
    cBOrigin->setCurrentIndex(plot->origin);
    cBReceiverPosition->setCurrentIndex(plot->receiverPosition);
    sBReferencePosition1->setValue(plot->ooPosition[0] * R2D);
    sBReferencePosition2->setValue(plot->ooPosition[1] * R2D);
    sBReferencePosition3->setValue(plot->ooPosition[2]);
    cBNavSys1->setChecked(plot->navSys & SYS_GPS);
    cBNavSys2->setChecked(plot->navSys & SYS_GLO);
    cBNavSys3->setChecked(plot->navSys & SYS_GAL);
    cBNavSys4->setChecked(plot->navSys & SYS_QZS);
    cBNavSys5->setChecked(plot->navSys & SYS_SBS);
    cBNavSys6->setChecked(plot->navSys & SYS_CMP);
    cBNavSys7->setChecked(plot->navSys & SYS_IRN);
    cBAnimationCycle->setCurrentText(QString::number(plot->animationCycle));
    sBRefreshCycle->setValue(plot->refreshCycle);
    cBHideLowSatellites->setCurrentIndex(plot->hideLowSatellites);
    cBElevationMask->setCurrentIndex(plot->elevationMaskP);
    lEExcludedSatellites->setText(plot->excludedSatellites);
    sBBufferSize->setValue(plot->rtBufferSize);
    cBTimeSync->setChecked(plot->timeSyncOut);
    sBTimeSync->setValue(plot->timeSyncPort);
    lERinexOptions->setText(plot->rinexOptions);
    lEShapeFile->setText(plot->shapeFile);
    lETLEFile->setText(plot->tleFile);
    lETLESatelliteFile->setText(plot->tleSatelliteFile);

    for (int i=0;i<cBYRange->count();i++) {
            double range;
            bool ok;
            QString unit;

            QString s=cBYRange->itemText(i);
            QStringList tokens = s.split(' ');
            if (tokens.length() != 2) continue;
            range = tokens.at(0).toInt(&ok);
            unit = tokens.at(1);
            if (!ok) continue;
            if (unit == "cm") range*=0.01;
            else if (unit == "km") range*=1000.0;
            if (range==plot->yRange) {
                cBYRange->setCurrentIndex(i);
                break;
            }
        }

    updateEnable();
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnOKClicked()
{
    QString s;
    double range;
    bool ok;
    QString unit;
    int marks[] = { 1, 2, 3, 4, 5, 10, 15, 20 };

    plot->timeLabel = cBTimeLabel->currentIndex();
    plot->latLonFormat = cBLatLonFormat->currentIndex();
    plot->autoScale = cBAutoScale->currentIndex();
    plot->showStats = cBShowStats->currentIndex();
    plot->showArrow = cBShowArrow->currentIndex();
    plot->showSlip = cBShowSlip->currentIndex();
    plot->showHalfC = cBShowHalfC->currentIndex();
    plot->showError = cBShowError->currentIndex();
    plot->showEphemeris = cBShowEphemeris->currentIndex();
    plot->showLabel = cBShowLabel->currentIndex();
    plot->showGridLabel = cBShowGridLabel->currentIndex();
    plot->showScale = cBShowScale->currentIndex();
    plot->showCompass = cBShowCompass->currentIndex();
    plot->plotStyle = cBPlotStyle->currentIndex();
    plot->markSize = marks[cBMarkSize->currentIndex()];

    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 2; j++)
            plot->mColor[j][i] = mColor[j][i];

    for (int i = 0; i < 4; i++)
        plot->cColor[i] = cColor[i];

    plot->fontName=fontOption.family();
    plot->fontSize=fontOption.pixelSize();
    plot->font = fontOption;

    plot->elevationMask = cBElevationMask->currentText().toDouble();
    plot->maxDop = cBMaxDop->currentText().toDouble();
    plot->maxMP = cBMaxMP->currentText().toDouble();
    plot->yRange = cBYRange->currentText().toDouble();
    plot->origin = cBOrigin->currentIndex();
    plot->receiverPosition = cBReceiverPosition->currentIndex();
    plot->ooPosition[0] = sBReferencePosition1->value() * D2R;
    plot->ooPosition[1] = sBReferencePosition2->value() * D2R;
    plot->ooPosition[2] = sBReferencePosition3->value();
    plot->navSys = (cBNavSys1->isChecked() ? SYS_GPS : 0) |
               (cBNavSys2->isChecked() ? SYS_GLO : 0) |
               (cBNavSys3->isChecked() ? SYS_GAL : 0) |
               (cBNavSys4->isChecked() ? SYS_QZS : 0) |
               (cBNavSys5->isChecked() ? SYS_SBS : 0) |
               (cBNavSys6->isChecked() ? SYS_CMP : 0) |
               (cBNavSys7->isChecked() ? SYS_IRN : 0);
    plot->animationCycle = cBAnimationCycle->currentText().toInt();
    plot->refreshCycle = sBRefreshCycle->value();
    plot->hideLowSatellites = cBHideLowSatellites->currentIndex();
    plot->elevationMaskP = cBElevationMask->currentIndex();
    plot->rtBufferSize = sBBufferSize->value();
    plot->timeSyncOut = cBTimeSync->isChecked();
    plot->timeSyncPort = sBTimeSync->value();
    plot->excludedSatellites = lEExcludedSatellites->text();
    plot->rinexOptions = lERinexOptions->text();
    plot->shapeFile = lEShapeFile->text();
    plot->tleFile = lETLEFile->text();
    plot->tleSatelliteFile = lETLESatelliteFile->text();

    s=cBYRange->currentText();
    QStringList tokens = s.split(' ');
    if (tokens.length() == 2) {
        range = tokens.at(0).toInt(&ok);
        unit = tokens.at(1);
        if (ok) {
            if (unit == "cm") range*=0.01;
            else if (unit == "km") range*=1000.0;
            plot->yRange=range;
        }
    }

    accept();
}
//---------------------------------------------------------------------------
void PlotOptDialog::mColorClicked()
{
    QToolButton *button = dynamic_cast<QToolButton *>(QObject::sender());

    if (!button) return;

    QColorDialog dialog(this);
    QColor *current = &mColor[0][1];

    if (button == lblMColor1) current = &mColor[0][1];
    if (button == lblMColor2) current = &mColor[0][2];
    if (button == lblMColor3) current = &mColor[0][3];
    if (button == lblMColor4) current = &mColor[0][4];
    if (button == lblMColor5) current = &mColor[0][5];
    if (button == lblMColor6) current = &mColor[0][6];
    if (button == lblMColor7) current = &mColor[1][1];
    if (button == lblMColor8) current = &mColor[1][2];
    if (button == lblMColor9) current = &mColor[1][3];
    if (button == lblMColor10) current = &mColor[1][4];
    if (button == lblMColor11) current = &mColor[1][5];
    if (button == lblMColor12) current = &mColor[1][6];
    dialog.setCurrentColor(*current);

    dialog.exec();
    if (dialog.result() != QDialog::Accepted) return;
    button->setStyleSheet(QString("background-color: %1").arg(color2String(dialog.currentColor())));
    *current = dialog.currentColor();
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnColor1Clicked()
{
    QColorDialog dialog(this);

    dialog.setCurrentColor(cColor[0]);
    dialog.exec();
    if (dialog.result() != QDialog::Accepted) return;
    lblColor1->setStyleSheet(QString("QLabel {background-color: %1;}").arg(color2String(dialog.currentColor())));
    cColor[0] = dialog.currentColor();
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnColor2Clicked()
{
    QColorDialog dialog(this);

    dialog.setCurrentColor(cColor[1]);
    dialog.exec();
    if (dialog.result() != QDialog::Accepted) return;
    lblColor2->setStyleSheet(QString("QLabel {background-color: %1;}").arg(color2String(dialog.currentColor())));
    cColor[1] = dialog.currentColor();
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnColor3Clicked()
{
    QColorDialog dialog(this);

    dialog.setCurrentColor(cColor[2]);
    dialog.exec();
    if (dialog.result() != QDialog::Accepted) return;
    lblColor3->setStyleSheet(QString("QLabel {background-color: %1;}").arg(color2String(dialog.currentColor())));
    cColor[2] = dialog.currentColor();
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnColor4Clicked()
{
    QColorDialog dialog(this);

    dialog.setCurrentColor(cColor[3]);
    dialog.exec();
    if (dialog.result() != QDialog::Accepted) return;
    lblColor4->setStyleSheet(QString("QLabel {background-color: %1;}").arg(color2String(dialog.currentColor())));
    cColor[3] = dialog.currentColor();
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnFontClicked()
{
    QFontDialog dialog(this);

    dialog.setCurrentFont(fontOption);
    dialog.exec();

    if (dialog.result() != QDialog::Accepted) return;
    fontOption = dialog.currentFont();
    updateFont();
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnShapeFileClicked()
{
    lEShapeFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open"))));
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnTLEFileClicked()
{
    lETLEFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open"), QString(), tr("Text Files (*.txt);;Position Files (*.pos *.snx);;All (*.*)"))));
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnTLESatelliteFileClicked()
{
    lETLESatelliteFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open"), QString(), tr("Text Files (*.txt);;Position Files (*.pos *.snx);;All (*.*)"))));
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnReferencePositionClicked()
{
    refDialog->roverPosition[0] = sBReferencePosition1->value();
    refDialog->roverPosition[1] = sBReferencePosition2->value();
    refDialog->roverPosition[2] = sBReferencePosition3->value();
    refDialog->move(pos().x() + size().width() / 2 - refDialog->size().width() / 2,
            pos().y() + size().height() / 2 - refDialog->size().height() / 2);
    refDialog->options=1;
    refDialog->exec();

    if (refDialog->result() != QDialog::Accepted) return;
    sBReferencePosition1->setValue(refDialog->position[0]);
    sBReferencePosition2->setValue(refDialog->position[1]);
    sBReferencePosition3->setValue(refDialog->position[2]);
}
//---------------------------------------------------------------------------
void PlotOptDialog::autoScaleChanged()
{
    updateEnable();
}
//---------------------------------------------------------------------------
void PlotOptDialog::originChanged()
{
    updateEnable();
}
//---------------------------------------------------------------------------
void PlotOptDialog::updateFont(void)
{
    lblFontName->setFont(fontOption);
    lblFontName->setText(fontOption.family() + " " + QString::number(fontOption.pointSize()) + " pt");
}
//---------------------------------------------------------------------------
void PlotOptDialog::updateEnable(void)
{
    sBReferencePosition1->setEnabled(cBOrigin->currentIndex() == 5 || cBReceiverPosition->currentIndex() == 1);
    sBReferencePosition2->setEnabled(cBOrigin->currentIndex() == 5 || cBReceiverPosition->currentIndex() == 1);
    sBReferencePosition3->setEnabled(cBOrigin->currentIndex() == 5 || cBReceiverPosition->currentIndex() == 1);
    lblReferencePosition->setEnabled(cBOrigin->currentIndex() == 5 || cBOrigin->currentIndex() == 6 || cBReceiverPosition->currentIndex() == 1);
    btnReferencePosition->setEnabled(cBOrigin->currentIndex() == 5 || cBOrigin->currentIndex() == 6 || cBReceiverPosition->currentIndex() == 1);
    sBTimeSync->setEnabled(cBTimeSync->isChecked());
}
//---------------------------------------------------------------------------
void PlotOptDialog::receiverPositionChanged()
{
    updateEnable();
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnTLEViewClicked()
{
    TextViewer *viewer;
    QString file = lETLEFile->text();

    if (file == "") return;
    viewer = new TextViewer(this);
    viewer->setWindowTitle(file);
    viewer->show();
    viewer->read(file);
}
//---------------------------------------------------------------------------
void PlotOptDialog::btnTLESatelliteViewClicked()
{
    TextViewer *viewer;
    QString file = lETLESatelliteFile->text();

    if (file == "") return;
    viewer = new TextViewer(this);
    viewer->setWindowTitle(file);
    viewer->show();
    viewer->read(file);
}
//---------------------------------------------------------------------------
void PlotOptDialog::timeSyncClicked()
{
    updateEnable();
}
//---------------------------------------------------------------------------
