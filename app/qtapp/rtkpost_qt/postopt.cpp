//---------------------------------------------------------------------------

#include "postmain.h"
#include "postopt.h"
#include "keydlg.h"
#include "viewer.h"
#include "refdlg.h"
#include "freqdlg.h"
#include "maskoptdlg.h"

#include <QShowEvent>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QCompleter>

extern MainForm *mainForm;

//---------------------------------------------------------------------------
OptDialog::OptDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    int nglo = MAXPRNGLO, ngal = MAXPRNGAL, nqzs = MAXPRNQZS, ncmp = MAXPRNCMP;
    int nirn = MAXPRNIRN;

    QString label;
    cBFrequencies->clear();
    for (int i = 0; i < NFREQ; i++) {
        label="L1";
        for (int j=1;j<=i;j++) {
            label+=QString("+%1").arg(j+1);
        }
        cBFrequencies->addItem(label);
    }

    QCompleter *fileCompleter = new QCompleter(this);
    QFileSystemModel *fileModel = new QFileSystemModel(fileCompleter);
    fileModel->setRootPath("");
    fileCompleter->setModel(fileModel);
    lEStationPositionFile->setCompleter(fileCompleter);
    lEAntennaPcvFile->setCompleter(fileCompleter);
    lESatellitePcvFile->setCompleter(fileCompleter);
    lEDCBFile->setCompleter(fileCompleter);
    lEGeoidDataFile->setCompleter(fileCompleter);
    lEEOPFile->setCompleter(fileCompleter);
    lEBLQFile->setCompleter(fileCompleter);
    lEIonosphereFile->setCompleter(fileCompleter);

    // station position file line edit actions
    QAction *acStationPositionFileSelect = lEStationPositionFile->addAction(QIcon(":/buttons/folder"), QLineEdit::TrailingPosition);
    acStationPositionFileSelect->setToolTip(tr("Select File"));
    QAction *acStationPositionFileView = lEStationPositionFile->addAction(QIcon(":/buttons/doc"), QLineEdit::TrailingPosition);
    acStationPositionFileView->setToolTip(tr("View File"));
    acStationPositionFileView->setEnabled(false);

    // satllite PCV line edit actions
    QAction *acSatellitePcvFileSelect = lESatellitePcvFile->addAction(QIcon(":/buttons/folder"), QLineEdit::TrailingPosition);
    acSatellitePcvFileSelect->setToolTip(tr("Select File"));
    QAction *acSatellitePcvFileView = lESatellitePcvFile->addAction(QIcon(":/buttons/doc"), QLineEdit::TrailingPosition);
    acSatellitePcvFileView->setToolTip(tr("View File"));
    acSatellitePcvFileView->setEnabled(false);

    // antenna PCV line edit actions
    QAction *acAntennaPcvFileSelect = lEAntennaPcvFile->addAction(QIcon(":/buttons/folder"), QLineEdit::TrailingPosition);
    acAntennaPcvFileSelect->setToolTip(tr("Select File"));
    QAction *acAntennaPcvFileView = lEAntennaPcvFile->addAction(QIcon(":/buttons/doc"), QLineEdit::TrailingPosition);
    acAntennaPcvFileView->setToolTip(tr("View File"));
    acAntennaPcvFileView->setEnabled(false);

    // DCB file line edit actions
    QAction *acDCBFileSelect = lEDCBFile->addAction(QIcon(":/buttons/folder"), QLineEdit::TrailingPosition);
    acDCBFileSelect->setToolTip(tr("Select File"));
    QAction *acDCBFileView = lEDCBFile->addAction(QIcon(":/buttons/doc"), QLineEdit::TrailingPosition);
    acDCBFileView->setToolTip(tr("View File"));
    acDCBFileView->setEnabled(false);

    // BLQ file line edit actions
    QAction *acBLQFileSelect = lEBLQFile->addAction(QIcon(":/buttons/folder"), QLineEdit::TrailingPosition);
    acBLQFileSelect->setToolTip(tr("Select File"));
    QAction *acBLQFileView = lEBLQFile->addAction(QIcon(":/buttons/doc"), QLineEdit::TrailingPosition);
    acBLQFileView->setToolTip(tr("View File"));
    acBLQFileView->setEnabled(false);

    // EOP file line edit actions
    QAction *acEOPFileSelect = lEEOPFile->addAction(QIcon(":/buttons/folder"), QLineEdit::TrailingPosition);
    acEOPFileSelect->setToolTip(tr("Select File"));
    QAction *acEOPFileView = lEEOPFile->addAction(QIcon(":/buttons/doc"), QLineEdit::TrailingPosition);
    acEOPFileView->setToolTip(tr("View File"));
    acEOPFileView->setEnabled(false);

    // geoid data file line edit actions
    QAction *acGeoidDataFileSelect = lEGeoidDataFile->addAction(QIcon(":/buttons/folder"), QLineEdit::TrailingPosition);
    acGeoidDataFileSelect->setToolTip(tr("Select File"));

    // Ionosphere file line edit actions
    QAction *acIonosphereFileSelect = lEIonosphereFile->addAction(QIcon(":/buttons/folder"), QLineEdit::TrailingPosition);
    acIonosphereFileSelect->setToolTip(tr("Select File"));

    connect(buttonBox, &QDialogButtonBox::accepted, this, &OptDialog::saveClose);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &OptDialog::reject);
    connect(cBRoverAntennaPcv, &QCheckBox::clicked, this, &OptDialog::updateEnable);
    connect(cBReferenceAntennaPcv, &QCheckBox::clicked, this, &OptDialog::updateEnable);
    connect(cBPositionMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(cBSolutionFormat, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(cBAmbiguityResolutionGPS, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(btnLoad, &QPushButton::clicked, this, &OptDialog::loadOptions);
    connect(btnSave, &QPushButton::clicked, this, &OptDialog::saveOptions);
    connect(cBFrequencies, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(btnReferencePosition, &QPushButton::clicked, this, &OptDialog::showReferencePositionDialog);
    connect(btnRoverPosition, &QPushButton::clicked, this, &OptDialog::showRoverPositionDialog);
    connect(acStationPositionFileView, &QAction::triggered, this, &OptDialog::viewStationPositionFile);
    connect(acStationPositionFileSelect, &QAction::triggered, this, &OptDialog::selectStationPositionFile);
    connect(lEStationPositionFile, &QLineEdit::textChanged, this, [acStationPositionFileView, this]()
            {acStationPositionFileView->setEnabled(QFile::exists(this->lEStationPositionFile->text()));});
    connect(cBOutputHeight, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(cBReferencePositionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(cBRoverPositionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(acSatellitePcvFileSelect, &QAction::triggered, this, &OptDialog::selectSatellitePcvFile);
    connect(acSatellitePcvFileView, &QAction::triggered, this, &OptDialog::viewSatelitePcvFile);
    connect(lESatellitePcvFile, &QLineEdit::textChanged, this, [acSatellitePcvFileView, this]()
            {acSatellitePcvFileView->setEnabled(QFile::exists(this->lESatellitePcvFile->text()));});
    connect(acAntennaPcvFileSelect, &QAction::triggered, this, &OptDialog::selectAntennaPcvFile);
    connect(acAntennaPcvFileView, &QAction::triggered, this, &OptDialog::viewAntennaPcvFile);
    connect(lEAntennaPcvFile, &QLineEdit::textChanged, this, [acAntennaPcvFileView, this]()
            {acAntennaPcvFileView->setEnabled(QFile::exists(this->lEAntennaPcvFile->text()));});
    connect(acDCBFileSelect, &QAction::triggered, this, &OptDialog::selectDCBFile);
    connect(acDCBFileView, &QAction::triggered, this, &OptDialog::viewDCBFile);
    connect(lEDCBFile, &QLineEdit::textChanged, this, [acDCBFileView, this]()
            {acDCBFileView->setEnabled(QFile::exists(this->lEDCBFile->text()));});
    connect(acBLQFileSelect, &QAction::triggered, this, &OptDialog::selectBLQFile);
    connect(acBLQFileView, &QAction::triggered, this, &OptDialog::viewBLQFile);
    connect(lEBLQFile, &QLineEdit::textChanged, this, [acBLQFileView, this]()
            {acBLQFileView->setEnabled(QFile::exists(this->lEBLQFile->text()));});
    connect(acEOPFileSelect, &QAction::triggered, this, &OptDialog::selectEOPFile);
    connect(acEOPFileView, &QAction::triggered, this, &OptDialog::viewEOPView);
    connect(lEEOPFile, &QLineEdit::textChanged, this, [acEOPFileView, this]()
            {acEOPFileView->setEnabled(QFile::exists(this->lEEOPFile->text()));});
    connect(acGeoidDataFileSelect, &QAction::triggered, this, &OptDialog::selectGeoidDataFile);
    connect(acIonosphereFileSelect, &QAction::triggered, this, &OptDialog::selectIonosphereFile);
    connect(cBSatelliteEphemeris, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(cBBaselineConstrain, &QCheckBox::clicked, this, &OptDialog::updateEnable);
    connect(cBNavSys1, &QCheckBox::clicked, this, &OptDialog::updateEnable);
    connect(cBNavSys2, &QCheckBox::clicked, this, &OptDialog::updateEnable);
    connect(cBNavSys3, &QCheckBox::clicked, this, &OptDialog::updateEnable);
    connect(cBNavSys4, &QCheckBox::clicked, this, &OptDialog::updateEnable);
    connect(cBNavSys5, &QCheckBox::clicked, this, &OptDialog::updateEnable);
    connect(cBNavSys6, &QCheckBox::clicked, this, &OptDialog::updateEnable);
    connect(cBIonosphereOption, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(cBTroposphereOption, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(cBDynamicModel, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(cBSatelliteEphemeris, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(cBRoverAntenna, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(cBReferenceAntenna, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptDialog::updateEnable);
    connect(btnHelp, &QPushButton::clicked, this, &OptDialog::showKeyDialog);
    connect(btnFrequencies, &QPushButton::clicked, this, &OptDialog::showFrequenciesDialog);
    connect(btnSnrMask, &QPushButton::clicked, this, &OptDialog::showMaskDialog);

    if (nglo <= 0) cBNavSys2->setEnabled(false);
    if (ngal <= 0) cBNavSys3->setEnabled(false);
    if (nqzs <= 0) cBNavSys4->setEnabled(false);
    if (ncmp <= 0) cBNavSys6->setEnabled(false);
    if (nirn <= 0) cBNavSys7->setEnabled(false);

    updateEnable();
}
//---------------------------------------------------------------------------
void OptDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    getOptions();

    tWOptions->setCurrentIndex(0);
}
//---------------------------------------------------------------------------
void OptDialog::saveClose()
{
	setOptions();

    accept();
}
//---------------------------------------------------------------------------
void OptDialog::loadOptions()
{
    load(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Load Options"), QString(), tr("Options File (*.conf);;All (*.*)"))));
}
//---------------------------------------------------------------------------
void OptDialog::saveOptions()
{
    QString file;

    file = QDir::toNativeSeparators(QFileDialog::getSaveFileName(this, tr("Save Options"), QString(), tr("Options File (*.conf);;All (*.*)")));

    QFileInfo f(file);
    if (f.suffix() == "") file = file + ".conf";
    save(file);
}
//---------------------------------------------------------------------------
void OptDialog::viewStationPositionFile()
{
    if (lEStationPositionFile->text() == "") return;

    TextViewer *viewer = new TextViewer(this);
    viewer->show();
    viewer->read(lEStationPositionFile->text());
}
//---------------------------------------------------------------------------
void OptDialog::selectStationPositionFile()
{
    lEStationPositionFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Station Position File"), lEStationPositionFile->text(), tr("Position File (*.pos);;All (*.*)"))));
}
//---------------------------------------------------------------------------
void OptDialog::roverPositionTypeChanged()
{
    QLineEdit *edit[] = { lERoverPosition1, lERoverPosition2, lERoverPosition3 };
	double pos[3];

    getPosition(roverPositionType, edit, pos);
    setPosition(cBRoverPositionType->currentIndex(), edit, pos);
    roverPositionType = cBRoverPositionType->currentIndex();

    updateEnable();
}
//---------------------------------------------------------------------------
void OptDialog::referencePositionTypeChanged()
{
    QLineEdit *edit[] = { lEReferencePosition1, lEReferencePosition2, lEReferencePosition3 };
	double pos[3];
    
    getPosition(referencePositionType, edit, pos);
    setPosition(cBReferencePositionType->currentIndex(), edit, pos);
    referencePositionType = cBReferencePositionType->currentIndex();

    updateEnable();
}
//---------------------------------------------------------------------------
void OptDialog::showRoverPositionDialog()
{
    QLineEdit *edit[] = { lERoverPosition1, lERoverPosition2, lERoverPosition3 };
    double p[3], pos[3];

    getPosition(cBRoverPositionType->currentIndex(), edit, p);
    ecef2pos(p, pos);

    RefDialog *refDialog = new RefDialog(this);
    refDialog->setRoverPosition(pos[0] * R2D, pos[1] * R2D, pos[2]);
    refDialog->stationPositionFile = lEStationPositionFile->text();
    refDialog->move(this->pos().x() + width() / 2 - refDialog->width() / 2,
            this->pos().y() + height() / 2 - refDialog->height() / 2);

    refDialog->exec();
    if (refDialog->result() != QDialog::Accepted) {
        delete refDialog;
        return;
    }

    pos[0] = refDialog->getPosition()[0] * D2R;
    pos[1] = refDialog->getPosition()[1] * D2R;
    pos[2] = refDialog->getPosition()[2];

    pos2ecef(pos, p);
    setPosition(cBRoverPositionType->currentIndex(), edit, p);

    delete refDialog;
}
//---------------------------------------------------------------------------
void OptDialog::showReferencePositionDialog()
{
    QLineEdit *edit[] = { lEReferencePosition1, lEReferencePosition2, lEReferencePosition3 };
    double p[3], pos[3];

    getPosition(cBReferencePositionType->currentIndex(), edit, p);
    ecef2pos(p, pos);

    RefDialog *refDialog = new RefDialog(this);
    refDialog->setRoverPosition(pos[0] * R2D, pos[1] * R2D, pos[2]);
    refDialog->stationPositionFile = lEStationPositionFile->text();
    refDialog->move(this->pos().x() + width() / 2 - refDialog->width() / 2,
            this->pos().y() + height() / 2 - refDialog->height() / 2);

    refDialog->exec();
    if (refDialog->result() != QDialog::Accepted) {
        delete refDialog;
        return;
    }

    pos[0] = refDialog->getPosition()[0] * D2R;
    pos[1] = refDialog->getPosition()[1] * D2R;
    pos[2] = refDialog->getPosition()[2];

    pos2ecef(pos, p);
    setPosition(cBReferencePositionType->currentIndex(), edit, p);

    delete refDialog;
}
//---------------------------------------------------------------------------
void OptDialog::viewSatelitePcvFile()
{
    if (lESatellitePcvFile->text() == "") return;

    TextViewer *viewer = new TextViewer(this);
    viewer->show();
    viewer->read(lESatellitePcvFile->text());
}
//---------------------------------------------------------------------------
void OptDialog::selectSatellitePcvFile()
{
    lESatellitePcvFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Satellite Antenna PCV File"), lESatellitePcvFile->text(), tr("PCV File (*.pcv *.atx);;All (*.*)"))));
}
//---------------------------------------------------------------------------
void OptDialog::viewAntennaPcvFile()
{
    if (lEAntennaPcvFile->text() == "") return;

    TextViewer *viewer = new TextViewer(this);
    viewer->show();
    viewer->read(lEAntennaPcvFile->text());
}
//---------------------------------------------------------------------------
void OptDialog::selectAntennaPcvFile()
{
    lEAntennaPcvFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Receiver Antenna PCV File"), lEAntennaPcvFile->text(), tr("APCV File (*.pcv *.atx);;All (*.*)"))));
    readAntennaList();
}
//---------------------------------------------------------------------------
void OptDialog::selectGeoidDataFile()
{
    lEGeoidDataFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Geoid Data File"), lEGeoidDataFile->text(), tr("All (*.*)"))));
}
//---------------------------------------------------------------------------
void OptDialog::selectDCBFile()
{
    lEDCBFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("DCB Data File"), lEDCBFile->text(), tr("DCB Data File (*.dcb *.DCB);;All (*.*)"))));
}
//---------------------------------------------------------------------------
void OptDialog::viewDCBFile()
{
    QString DCBFile_Text = lEDCBFile->text();

    if (DCBFile_Text == "") return;

    TextViewer *viewer = new TextViewer(this);
    viewer->show();
	viewer->read(DCBFile_Text);
}
//---------------------------------------------------------------------------
void OptDialog::selectEOPFile()
{
    lEEOPFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("EOP Date File"), lEEOPFile->text(), tr("EOP Data File (*.eop *.erp);;All (*.*)"))));
}
//---------------------------------------------------------------------------
void OptDialog::viewEOPView()
{
    QString EOPFile_Text = lEEOPFile->text();

    if (EOPFile_Text == "") return;

    TextViewer *viewer = new TextViewer(this);
    viewer->show();
	viewer->read(EOPFile_Text);
}
//---------------------------------------------------------------------------
void OptDialog::selectBLQFile()
{
    lEBLQFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Ocean Tide Loading BLQ File"), lEBLQFile->text(), tr("OTL BLQ File (*.blq);;All (*.*)"))));
}
//---------------------------------------------------------------------------
void OptDialog::viewBLQFile()
{
    QString BLQFile_Text = lEBLQFile->text();

    if (BLQFile_Text == "") return;

    TextViewer *viewer = new TextViewer(this);
    viewer->show();
	viewer->read(BLQFile_Text);
}
//---------------------------------------------------------------------------
void OptDialog::selectIonosphereFile()
{
    lEIonosphereFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Ionosphere Data File"), lEIonosphereFile->text(), tr("Ionosphere Data File (*.*i,*stec);;All (*.*)"))));
}
//---------------------------------------------------------------------------
void OptDialog::getOptions(void)
{
    QLineEdit *editu[] = { lERoverPosition1, lERoverPosition2, lERoverPosition3 };
    QLineEdit *editr[] = { lEReferencePosition1, lEReferencePosition2, lEReferencePosition3 };

    cBPositionMode->setCurrentIndex(mainForm->positionMode);
    cBFrequencies->setCurrentIndex(mainForm->frequencies);
    cBSolution->setCurrentIndex(mainForm->solution);
    cBElevationMask->setCurrentText(QString::number(mainForm->elevationMask, 'f', 0));
    snrMask = mainForm->snrMask;
    cBDynamicModel->setCurrentIndex(mainForm->dynamicModel);
    cBTideCorrection->setCurrentIndex(mainForm->tideCorrection);
    cBIonosphereOption->setCurrentIndex(mainForm->ionosphereOption);
    cBTroposphereOption->setCurrentIndex(mainForm->troposphereOption);
    cBSatelliteEphemeris->setCurrentIndex(mainForm->satelliteEphemeris);
    lEExcludedSatellites->setText(mainForm->excludedSatellites);
    cBNavSys1->setChecked(mainForm->navigationSystems & SYS_GPS);
    cBNavSys2->setChecked(mainForm->navigationSystems & SYS_GLO);
    cBNavSys3->setChecked(mainForm->navigationSystems & SYS_GAL);
    cBNavSys4->setChecked(mainForm->navigationSystems & SYS_QZS);
    cBNavSys5->setChecked(mainForm->navigationSystems & SYS_SBS);
    cBNavSys6->setChecked(mainForm->navigationSystems & SYS_CMP);
    cBNavSys7->setChecked(mainForm->navigationSystems & SYS_IRN);
    cBPositionOption1->setChecked(mainForm->positionOption[0]);
    cBPositionOption2->setChecked(mainForm->positionOption[1]);
    cBPositionOption3->setChecked(mainForm->positionOption[2]);
    cBPositionOption4->setChecked(mainForm->positionOption[3]);
    cBPositionOption5->setChecked(mainForm->positionOption[4]);
    cBPositionOption6->setChecked(mainForm->positionOption[5]);

    cBAmbiguityResolutionGPS->setCurrentIndex(mainForm->ambiguityResolutionGPS);
    cBAmbiguityResolutionGLO->setCurrentIndex(mainForm->ambiguityResolutionGLO);
    cBAmbiguityResolutionBDS->setCurrentIndex(mainForm->ambiguityResolutionBDS);
    sBValidThresAR->setValue(mainForm->validThresAR);
    sBValidThresARMin->setValue(mainForm->validThresARMin);
    sBValidThresARMax->setValue(mainForm->validThresARMax);
    sBGlonassHwBias->setValue(mainForm->glonassHwBias);
    sBOutageCountResetAmbiguity->setValue(mainForm->outputCntResetAmbiguity);
    sBFixCountHoldAmbiguity->setValue(mainForm->fixCntHoldAmbiguity);
    sBLockCountFixAmbiguity->setValue(mainForm->lockCntFixAmbiguity);
    sBElevationMaskAR->setValue(mainForm->elevationMaskAR);
    sBElevationMaskHold->setValue(mainForm->elevationMaskHold);
    sBMaxAgeDifferences->setValue(mainForm->maxAgeDiff);
    sBRejectCode->setValue(mainForm->rejectCode);
    sBRejectPhase->setValue(mainForm->rejectPhase);
    sBVarHoldAmb->setValue(mainForm->varHoldAmb);
    sBGainHoldAmb->setValue(mainForm->gainHoldAmb);
    sBSlipThreshold->setValue(mainForm->slipThreshold);
    sBDopplerThreshold->setValue(mainForm->dopplerThreshold);
    //sBARIter->setValue(mainForm->ARIter);
    sBNumIteration->setValue(mainForm->numIter);
    sBMinFixSats->setValue(mainForm->minFixSats);
    sBMinHoldSats->setValue(mainForm->minHoldSats);
    sBMinDropSats->setValue(mainForm->minDropSats);
    sBMaxPositionVarAR->setValue(mainForm->maxPositionVarAR);
    cBARFilter->setCurrentIndex(mainForm->ARFilter);
    sBBaselineLen->setValue(mainForm->baseLine[0]);
    sBBaselineSig->setValue(mainForm->baseLine[1]);
    cBBaselineConstrain->setChecked(mainForm->baseLineConstrain);

    cBSolutionFormat->setCurrentIndex(mainForm->solutionFormat);
    cBTimeFormat->setCurrentIndex(mainForm->timeFormat);
    sBTimeDecimal->setValue(mainForm->timeDecimal);
    cBLatLonFormat->setCurrentIndex(mainForm->latLonFormat);
    lEFieldSeperator->setText(mainForm->fieldSeperator);
    cBOutputHeader->setCurrentIndex(mainForm->outputHeader);
    cBOutputOptions->setCurrentIndex(mainForm->outputOptions);
    cBOutputVelocity->setCurrentIndex(mainForm->outputVelocity);
    cBOutputSingle->setCurrentIndex(mainForm->outputSingle);
    sBMaxSolutionStd->setValue(mainForm->maxSolutionStd);
    cBOutputDatum->setCurrentIndex(mainForm->outputDatum);
    cBOutputHeight->setCurrentIndex(mainForm->outputHeight);
    cBOutputGeoid->setCurrentIndex(mainForm->outputGeoid);
    cBSolutionStatic->setCurrentIndex(mainForm->solutionStatic);
    cBDebugTrace->setCurrentIndex(mainForm->debugTrace);
    cBDebugStatus->setCurrentIndex(mainForm->debugStatus);

    sBMeasurementErrorR1->setValue(mainForm->measurementErrorR1);
    sBMeasurementErrorR2->setValue(mainForm->measurementErrorR2);
    sBMeasurementErrorR5->setValue(mainForm->measurementErrorR5);
    sBMeasurementError2->setValue(mainForm->measurementError2);
    sBMeasurementError3->setValue(mainForm->measurementError3);
    sBMeasurementError4->setValue(mainForm->measurementError4);
    sBMeasurementError5->setValue(mainForm->measurementError5);
    sBSatelliteClockStability->setValue(mainForm->satelliteClockStability);
    sBProcessNoise1->setValue(mainForm->processNoise1);
    sBProcessNoise2->setValue(mainForm->processNoise2);
    sBProcessNoise3->setValue(mainForm->processNoise3);
    sBProcessNoise4->setValue(mainForm->processNoise4);
    sBProcessNoise5->setValue(mainForm->processNoise5);

    cBRoverAntennaPcv->setChecked(mainForm->roverAntennaPcv);
    cBReferenceAntennaPcv->setChecked(mainForm->referenceAntennaPcv);
    sBRoverAntennaE->setValue(mainForm->roverAntennaE);
    sBRoverAntennaN->setValue(mainForm->roverAntennaN);
    sBRoverAntennaU->setValue(mainForm->roverAntennaU);
    sBReferenceAntennaE->setValue(mainForm->referenceAntennaE);
    sBReferenceAntennaN->setValue(mainForm->referenceAntennaN);
    sBReferenceAntennaU->setValue(mainForm->referenceAntennaU);
    lEAntennaPcvFile->setText(mainForm->antennaPcvFile);

    lERnxOptions1->setText(mainForm->rnxOptions1);
    lERnxOptions2->setText(mainForm->rnxOptions2);
    lEPPPOptions->setText(mainForm->pppOptions);

    cBIntputReferenceObservation->setCurrentIndex(mainForm->intpolateReferenceObs);
    lESbasSat->setText(QString::number(mainForm->sbasSat));
    lESatellitePcvFile->setText(mainForm->satellitePcvFile);
    lEStationPositionFile->setText(mainForm->stationPositionFile);
    lEGeoidDataFile->setText(mainForm->geoidDataFile);
    lEEOPFile->setText(mainForm->eopFile);
    lEDCBFile->setText(mainForm->dcbFile);
    lEBLQFile->setText(mainForm->blqFile);
    lEIonosphereFile->setText(mainForm->ionosphereFile);
    cBRoverPositionType->setCurrentIndex(mainForm->roverPositionType);
    cBReferencePositionType->setCurrentIndex(mainForm->referencePositionType);
    roverPositionType = cBRoverPositionType->currentIndex();
    referencePositionType = cBReferencePositionType->currentIndex();
    setPosition(cBRoverPositionType->currentIndex(), editu, mainForm->roverPosition);
    setPosition(cBReferencePositionType->currentIndex(), editr, mainForm->referencePosition);
	readAntennaList();

    cBRoverAntenna->setCurrentText(mainForm->roverAntenna);
    cBReferenceAntenna->setCurrentText(mainForm->referenceAntenna);

    tERoverList->setPlainText(mainForm->roverList);
    tEBaseList->setPlainText(mainForm->baseList);

	updateEnable();
}
//---------------------------------------------------------------------------
void OptDialog::setOptions(void)
{
    QLineEdit *editu[] = { lERoverPosition1, lERoverPosition2, lERoverPosition3 };
    QLineEdit *editr[] = { lEReferencePosition1, lEReferencePosition2, lEReferencePosition3 };

    mainForm->positionMode = cBPositionMode->currentIndex();
    mainForm->frequencies = cBFrequencies->currentIndex();
    mainForm->solution = cBSolution->currentIndex();
    mainForm->elevationMask = cBElevationMask->currentText().toDouble();
    mainForm->snrMask = snrMask;
    mainForm->dynamicModel = cBDynamicModel->currentIndex();
    mainForm->tideCorrection = cBTideCorrection->currentIndex();
    mainForm->ionosphereOption = cBIonosphereOption->currentIndex();
    mainForm->troposphereOption = cBTroposphereOption->currentIndex();
    mainForm->satelliteEphemeris = cBSatelliteEphemeris->currentIndex();
    mainForm->excludedSatellites = lEExcludedSatellites->text();

    mainForm->ambiguityResolutionGPS = cBAmbiguityResolutionGPS->currentIndex();
    mainForm->ambiguityResolutionGLO = cBAmbiguityResolutionGLO->currentIndex();
    mainForm->ambiguityResolutionBDS = cBAmbiguityResolutionBDS->currentIndex();
    mainForm->validThresAR = sBValidThresAR->value();
    mainForm->maxPositionVarAR = sBMaxPositionVarAR->value();
    mainForm->glonassHwBias = sBGlonassHwBias->value();
    mainForm->validThresARMin = sBValidThresARMin->value();
    mainForm->validThresARMax = sBValidThresARMax->value();
    mainForm->outputCntResetAmbiguity = sBOutageCountResetAmbiguity->value();
    mainForm->lockCntFixAmbiguity = sBLockCountFixAmbiguity->value();
    mainForm->fixCntHoldAmbiguity = sBFixCountHoldAmbiguity->value();
    mainForm->elevationMaskAR = sBElevationMaskAR->value();
    mainForm->elevationMaskHold = sBElevationMaskHold->value();
    mainForm->maxAgeDiff = sBMaxAgeDifferences->value();
    mainForm->rejectPhase = sBRejectPhase->value();
    mainForm->rejectCode = sBRejectCode->value();
    mainForm->varHoldAmb = sBVarHoldAmb->value();
    mainForm->gainHoldAmb = sBGainHoldAmb->value();
    mainForm->slipThreshold = sBSlipThreshold->value();
    mainForm->dopplerThreshold = sBDopplerThreshold->value();
    //mainForm->ARIter = sBARIter->value();
    mainForm->numIter = sBNumIteration->value();
    mainForm->minFixSats = sBMinFixSats->value();
    mainForm->minHoldSats = sBMinHoldSats->value();
    mainForm->minDropSats = sBMinHoldSats->value();
    mainForm->ARFilter = cBARFilter->currentIndex();
    mainForm->baseLineConstrain = cBBaselineConstrain->isChecked();
    mainForm->baseLine[0] = sBBaselineLen->value();
    mainForm->baseLine[1] = sBBaselineSig->value();

    mainForm->navigationSystems = 0;
    if (cBNavSys1->isChecked()) mainForm->navigationSystems |= SYS_GPS;
    if (cBNavSys2->isChecked()) mainForm->navigationSystems |= SYS_GLO;
    if (cBNavSys3->isChecked()) mainForm->navigationSystems |= SYS_GAL;
    if (cBNavSys4->isChecked()) mainForm->navigationSystems |= SYS_QZS;
    if (cBNavSys5->isChecked()) mainForm->navigationSystems |= SYS_SBS;
    if (cBNavSys6->isChecked()) mainForm->navigationSystems |= SYS_CMP;
    if (cBNavSys7->isChecked()) mainForm->navigationSystems |= SYS_IRN;
    mainForm->positionOption[0] = cBPositionOption1->isChecked();
    mainForm->positionOption[1] = cBPositionOption2->isChecked();
    mainForm->positionOption[2] = cBPositionOption3->isChecked();
    mainForm->positionOption[3] = cBPositionOption4->isChecked();
    mainForm->positionOption[4] = cBPositionOption5->isChecked();
    mainForm->positionOption[5] = cBPositionOption6->isChecked();

    mainForm->solutionFormat = cBSolutionFormat->currentIndex();
    mainForm->timeFormat = cBTimeFormat->currentIndex();
    mainForm->timeDecimal = sBTimeDecimal->value();
    mainForm->latLonFormat = cBLatLonFormat->currentIndex();
    mainForm->fieldSeperator = lEFieldSeperator->text();
    mainForm->outputHeader = cBOutputHeader->currentIndex();
    mainForm->outputOptions = cBOutputOptions->currentIndex();
    mainForm->outputVelocity = cBOutputVelocity->currentIndex();
    mainForm->outputSingle = cBOutputSingle->currentIndex();
    mainForm->maxSolutionStd = sBMaxSolutionStd->value();
    mainForm->outputDatum = cBOutputDatum->currentIndex();
    mainForm->outputHeight = cBOutputHeight->currentIndex();
    mainForm->outputGeoid = cBOutputGeoid->currentIndex();
    mainForm->solutionStatic = cBSolutionStatic->currentIndex();
    mainForm->debugTrace = cBDebugTrace->currentIndex();
    mainForm->debugStatus = cBDebugStatus->currentIndex();

    mainForm->measurementErrorR1 = sBMeasurementErrorR1->value();
    mainForm->measurementErrorR2 = sBMeasurementErrorR2->value();
    mainForm->measurementErrorR5 = sBMeasurementErrorR5->value();
    mainForm->measurementError2 = sBMeasurementError2->value();
    mainForm->measurementError3 = sBMeasurementError3->value();
    mainForm->measurementError4 = sBMeasurementError4->value();
    mainForm->measurementError5 = sBMeasurementError5->value();
    mainForm->processNoise1 = sBProcessNoise1->value();
    mainForm->processNoise2 = sBProcessNoise2->value();
    mainForm->processNoise3 = sBProcessNoise3->value();
    mainForm->processNoise4 = sBProcessNoise4->value();
    mainForm->processNoise5 = sBProcessNoise5->value();
    mainForm->satelliteClockStability = sBSatelliteClockStability->value();

    mainForm->roverPositionType = cBRoverPositionType->currentIndex();
    mainForm->referencePositionType = cBReferencePositionType->currentIndex();
    mainForm->roverAntennaPcv = cBRoverAntennaPcv->isChecked();
    mainForm->referenceAntennaPcv = cBReferenceAntennaPcv->isChecked();
    mainForm->roverAntenna = cBRoverAntenna->currentText();
    mainForm->referenceAntenna = cBReferenceAntenna->currentText();
    mainForm->roverAntennaE = sBRoverAntennaE->value();
    mainForm->roverAntennaN = sBRoverAntennaN->value();
    mainForm->roverAntennaU = sBRoverAntennaU->value();
    mainForm->referenceAntennaE = sBReferenceAntennaE->value();
    mainForm->referenceAntennaN = sBReferenceAntennaN->value();
    mainForm->referenceAntennaU = sBReferenceAntennaU->value();

    mainForm->rnxOptions1 = lERnxOptions1->text();
    mainForm->rnxOptions2 = lERnxOptions2->text();
    mainForm->pppOptions = lEPPPOptions->text();

    mainForm->intpolateReferenceObs = cBIntputReferenceObservation->currentIndex();
    mainForm->sbasSat = lESbasSat->text().toInt();
    mainForm->antennaPcvFile = lEAntennaPcvFile->text();
    mainForm->satellitePcvFile = lESatellitePcvFile->text();
    mainForm->stationPositionFile = lEStationPositionFile->text();
    mainForm->geoidDataFile = lEGeoidDataFile->text();
    mainForm->eopFile = lEEOPFile->text();
    mainForm->dcbFile = lEDCBFile->text();
    mainForm->blqFile = lEBLQFile->text();
    mainForm->ionosphereFile = lEIonosphereFile->text();
    getPosition(cBRoverPositionType->currentIndex(), editu, mainForm->roverPosition);
    getPosition(cBReferencePositionType->currentIndex(), editr, mainForm->referencePosition);

    mainForm->roverList = tERoverList->toPlainText();
    mainForm->baseList = tEBaseList->toPlainText();

	updateEnable();
}
//---------------------------------------------------------------------------
void OptDialog::load(const QString &file)
{
    QLineEdit *editu[] = { lERoverPosition1, lERoverPosition2, lERoverPosition3 };
    QLineEdit *editr[] = { lEReferencePosition1, lEReferencePosition2, lEReferencePosition3 };
    QString buff;
    char id[32];
    int sat;
    prcopt_t prcopt = prcopt_default;
    solopt_t solopt = solopt_default;
    filopt_t filopt;

    memset(&filopt, 0, sizeof(filopt_t));

    resetsysopts();
    if (!loadopts(qPrintable(file), sysopts)) return;
    getsysopts(&prcopt, &solopt, &filopt);

    cBPositionMode->setCurrentIndex(prcopt.mode);
    cBFrequencies->setCurrentIndex(prcopt.nf > NFREQ - 1 ? NFREQ - 1 : prcopt.nf - 1);
    cBSolution->setCurrentIndex(prcopt.soltype);
    cBElevationMask->setCurrentIndex(cBElevationMask->findText(QString::number(prcopt.elmin * R2D, 'f', 0)));
    snrMask = prcopt.snrmask;
    cBDynamicModel->setCurrentIndex(prcopt.dynamics);
    cBTideCorrection->setCurrentIndex(prcopt.tidecorr);
    cBIonosphereOption->setCurrentIndex(prcopt.ionoopt);
    cBTroposphereOption->setCurrentIndex(prcopt.tropopt);
    cBSatelliteEphemeris->setCurrentIndex(prcopt.sateph);
    lEExcludedSatellites->setText("");
    for (sat = 1; sat <= MAXSAT; sat++) {
        if (!prcopt.exsats[sat - 1]) continue;
        satno2id(sat, id);
        buff += QString("%1%2%3").arg(buff.isEmpty() ? "" : " ").arg(prcopt.exsats[sat - 1] == 2 ? "+" : "").arg(id);
    }
    lEExcludedSatellites->setText(buff);
    cBNavSys1->setChecked(prcopt.navsys & SYS_GPS);
    cBNavSys2->setChecked(prcopt.navsys & SYS_GLO);
    cBNavSys3->setChecked(prcopt.navsys & SYS_GAL);
    cBNavSys4->setChecked(prcopt.navsys & SYS_QZS);
    cBNavSys5->setChecked(prcopt.navsys & SYS_SBS);
    cBNavSys6->setChecked(prcopt.navsys & SYS_CMP);
    cBNavSys7->setChecked(prcopt.navsys & SYS_IRN);
    cBPositionOption1->setChecked(prcopt.posopt[0]);
    cBPositionOption2->setChecked(prcopt.posopt[1]);
    cBPositionOption3->setChecked(prcopt.posopt[2]);
    cBPositionOption4->setChecked(prcopt.posopt[3]);
    cBPositionOption5->setChecked(prcopt.posopt[4]);
    cBPositionOption6->setChecked(prcopt.posopt[5]);

    cBAmbiguityResolutionGPS->setCurrentIndex(prcopt.modear);
    cBAmbiguityResolutionGLO->setCurrentIndex(prcopt.glomodear);
    cBAmbiguityResolutionBDS->setCurrentIndex(prcopt.bdsmodear);
    sBValidThresAR->setValue(prcopt.thresar[0]);
    sBMaxPositionVarAR->setValue(prcopt.thresar[1]);
    sBGlonassHwBias->setValue(prcopt.thresar[2]);
    sBValidThresARMin->setValue(prcopt.thresar[5]);
    sBValidThresARMax->setValue(prcopt.thresar[6]);
    sBOutageCountResetAmbiguity->setValue(prcopt.maxout);
    sBFixCountHoldAmbiguity->setValue(prcopt.minfix);
    sBLockCountFixAmbiguity->setValue(prcopt.minlock);
    sBElevationMaskAR->setValue(prcopt.elmaskar * R2D);
    sBElevationMaskHold->setValue(prcopt.elmaskhold * R2D);
    sBMaxAgeDifferences->setValue(prcopt.maxtdiff);
    sBRejectCode->setValue(prcopt.maxinno[0]);
    sBRejectPhase->setValue(prcopt.maxinno[1]);
    sBVarHoldAmb->setValue(prcopt.varholdamb);
    sBGainHoldAmb->setValue(prcopt.gainholdamb);
    sBSlipThreshold->setValue(prcopt.thresslip);
    sBDopplerThreshold->setValue(prcopt.thresdop);
    //sBARIter->setValue(prcopt.armaxiter);
    sBMinFixSats->setValue(prcopt.minfixsats);
    sBMinHoldSats->setValue(prcopt.minholdsats);
    sBMinDropSats->setValue(prcopt.mindropsats);
    sBNumIteration->setValue(prcopt.niter);
    cBSyncSolution->setCurrentIndex(prcopt.syncsol);
    cBARFilter->setCurrentIndex(prcopt.arfilter);
    sBBaselineLen->setValue(prcopt.baseline[0]);
    sBBaselineSig->setValue(prcopt.baseline[1]);
    cBBaselineConstrain->setChecked(prcopt.baseline[0] > 0.0);

    cBSolutionFormat->setCurrentIndex(solopt.posf);
    cBTimeFormat->setCurrentIndex(solopt.timef == 0 ? 0 : solopt.times + 1);
    sBTimeDecimal->setValue(solopt.timeu);
    cBLatLonFormat->setCurrentIndex(solopt.degf);
    lEFieldSeperator->setText(solopt.sep);
    cBOutputHeader->setCurrentIndex(solopt.outhead);
    cBOutputOptions->setCurrentIndex(solopt.outopt);
    cBOutputVelocity->setCurrentIndex(solopt.outvel);
    cBOutputSingle->setCurrentIndex(prcopt.outsingle);
    sBMaxSolutionStd->setValue(solopt.maxsolstd);
    cBOutputDatum->setCurrentIndex(solopt.datum);
    cBOutputHeight->setCurrentIndex(solopt.height);
    cBOutputGeoid->setCurrentIndex(solopt.geoid);
    cBSolutionStatic->setCurrentIndex(solopt.solstatic);
    sBNmeaInterval1->setValue(solopt.nmeaintv[0]);
    sBNmeaInterval2->setValue(solopt.nmeaintv[1]);
    cBDebugTrace->setCurrentIndex(solopt.trace);
    cBDebugStatus->setCurrentIndex(solopt.sstat);

    sBMeasurementErrorR1->setValue(prcopt.eratio[0]);
    sBMeasurementErrorR2->setValue(prcopt.eratio[1]);
    sBMeasurementErrorR5->setValue(prcopt.eratio[2]);
    sBMeasurementError2->setValue(prcopt.err[1]);
    sBMeasurementError3->setValue(prcopt.err[2]);
    sBMeasurementError4->setValue(prcopt.err[3]);
    sBMeasurementError5->setValue(prcopt.err[4]);
    sBSatelliteClockStability->setValue(prcopt.sclkstab);
    sBProcessNoise1->setValue(prcopt.prn[0]);
    sBProcessNoise2->setValue(prcopt.prn[1]);
    sBProcessNoise3->setValue(prcopt.prn[2]);
    sBProcessNoise4->setValue(prcopt.prn[3]);
    sBProcessNoise5->setValue(prcopt.prn[4]);

    cBRoverAntennaPcv->setChecked(*prcopt.anttype[0]);
    cBReferenceAntennaPcv->setChecked(*prcopt.anttype[1]);
    cBRoverAntenna->setCurrentIndex(cBRoverAntenna->findText(prcopt.anttype[0]));
    cBReferenceAntenna->setCurrentIndex(cBReferenceAntenna->findText(prcopt.anttype[1]));
    sBRoverAntennaE->setValue(prcopt.antdel[0][0]);
    sBRoverAntennaN->setValue(prcopt.antdel[0][1]);
    sBRoverAntennaU->setValue(prcopt.antdel[0][2]);
    sBReferenceAntennaE->setValue(prcopt.antdel[1][0]);
    sBReferenceAntennaN->setValue(prcopt.antdel[1][1]);
    sBReferenceAntennaU->setValue(prcopt.antdel[1][2]);

    lERnxOptions1->setText(prcopt.rnxopt[0]);
    lERnxOptions2->setText(prcopt.rnxopt[1]);
    lEPPPOptions->setText(prcopt.pppopt);

    cBIntputReferenceObservation->setCurrentIndex(prcopt.intpref);
    lESbasSat->setText(QString::number(prcopt.sbassatsel));
    cBRoverPositionType->setCurrentIndex(prcopt.rovpos == 0 ? 0 : prcopt.rovpos + 2);
    cBReferencePositionType->setCurrentIndex(prcopt.refpos == 0 ? 0 : prcopt.refpos + 2);
    roverPositionType = cBRoverPositionType->currentIndex();
    referencePositionType = cBReferencePositionType->currentIndex();
    setPosition(cBRoverPositionType->currentIndex(), editu, prcopt.ru);
    setPosition(cBReferencePositionType->currentIndex(), editr, prcopt.rb);

    lESatellitePcvFile->setText(filopt.satantp);
    lEAntennaPcvFile->setText(filopt.rcvantp);
    lEStationPositionFile->setText(filopt.stapos);
    lEGeoidDataFile->setText(filopt.geoid);
    lEEOPFile->setText(filopt.eop);
    lEDCBFile->setText(filopt.dcb);
    lEBLQFile->setText(filopt.blq);
    lEIonosphereFile->setText(filopt.iono);

	readAntennaList();
	updateEnable();
}

//---------------------------------------------------------------------------
void OptDialog::save(const QString &file)
{
    QString ExSats_Text = lEExcludedSatellites->text(), FieldSep_Text = lEFieldSeperator->text();
    QString RovAnt_Text = cBRoverAntenna->currentText(), RefAnt_Text = cBReferenceAntenna->currentText();
    QString SatPcvFile_Text = lESatellitePcvFile->text();
    QString AntPcvFile_Text = lEAntennaPcvFile->text();
    QString lEStationPositionFile_Text = lEStationPositionFile->text();
    QString GeoidDataFile_Text = lEGeoidDataFile->text();
    QString EOPFile_Text = lEEOPFile->text();
    QString DCBFile_Text = lEDCBFile->text();
    QString BLQFile_Text = lEBLQFile->text();
    QString IonoFile_Text = lEIonosphereFile->text();
    QString RnxOpts1_Text = lERnxOptions1->text();
    QString RnxOpts2_Text = lERnxOptions2->text();
    QString PPPOpts_Text = lEPPPOptions->text();
    QLineEdit *editu[] = { lERoverPosition1, lERoverPosition2, lERoverPosition3 };
    QLineEdit *editr[] = { lEReferencePosition1, lEReferencePosition2, lEReferencePosition3 };
    char buff[1024], *p, comment[256], s[64];
    int sat, ex;
    prcopt_t prcopt = prcopt_default;
    solopt_t solopt = solopt_default;
    filopt_t filopt;

    memset(&filopt, 0, sizeof(filopt_t));

    prcopt.mode = cBPositionMode->currentIndex();
    prcopt.nf = cBFrequencies->currentIndex() + 1;
    prcopt.soltype = cBSolution->currentIndex();
    prcopt.elmin = cBElevationMask->currentText().toDouble() * D2R;
    prcopt.snrmask = snrMask;
    prcopt.dynamics = cBDynamicModel->currentIndex();
    prcopt.tidecorr = cBTideCorrection->currentIndex();
    prcopt.ionoopt = cBIonosphereOption->currentIndex();
    prcopt.tropopt = cBTroposphereOption->currentIndex();
    prcopt.sateph = cBSatelliteEphemeris->currentIndex();
    if (lEExcludedSatellites->text() != "") {
        strncpy(buff, qPrintable(ExSats_Text), 1023);
        for (p = strtok(buff, " "); p; p = strtok(NULL, " ")) {
            if (*p == '+') {
                ex = 2; p++;
            } else {
                ex = 1;
            }
            if (!(sat = satid2no(p))) continue;
            prcopt.exsats[sat - 1] = (unsigned char)ex;
        }
    }
    prcopt.navsys = (cBNavSys1->isChecked() ? SYS_GPS : 0) |
                    (cBNavSys2->isChecked() ? SYS_GLO : 0) |
                    (cBNavSys3->isChecked() ? SYS_GAL : 0) |
                    (cBNavSys4->isChecked() ? SYS_QZS : 0) |
                    (cBNavSys5->isChecked() ? SYS_SBS : 0) |
                    (cBNavSys6->isChecked() ? SYS_CMP : 0) |
                    (cBNavSys7->isChecked() ? SYS_IRN : 0);
    prcopt.posopt[0] = cBPositionOption1->isChecked();
    prcopt.posopt[1] = cBPositionOption2->isChecked();
    prcopt.posopt[2] = cBPositionOption3->isChecked();
    prcopt.posopt[3] = cBPositionOption4->isChecked();
    prcopt.posopt[4] = cBPositionOption5->isChecked();
    prcopt.posopt[5] = cBPositionOption6->isChecked();
//	prcopt.mapfunc=MapFunc->currentIndex();

    prcopt.modear = cBAmbiguityResolutionGPS->currentIndex();
    prcopt.glomodear = cBAmbiguityResolutionGLO->currentIndex();
    prcopt.bdsmodear = cBAmbiguityResolutionBDS->currentIndex();
    prcopt.thresar[0] = sBValidThresAR->value();
    prcopt.thresar[1] = sBMaxPositionVarAR->value();
    prcopt.thresar[2] = sBGlonassHwBias->value();
    prcopt.thresar[5] = sBValidThresARMin->value();
    prcopt.thresar[6] = sBValidThresARMax->value();
    prcopt.maxout = sBOutageCountResetAmbiguity->value();
    prcopt.minfix = sBFixCountHoldAmbiguity->value();
    prcopt.minlock = sBLockCountFixAmbiguity->value();
    prcopt.elmaskar = sBElevationMaskAR->value() * D2R;
    prcopt.elmaskhold = sBElevationMaskHold->value() * D2R;
    prcopt.maxtdiff = sBMaxAgeDifferences->value();
    prcopt.maxinno[0] = sBRejectCode->value();
    prcopt.maxinno[1] = sBRejectPhase->value();
    prcopt.varholdamb = sBVarHoldAmb->value();
    prcopt.gainholdamb = sBGainHoldAmb->value();
    prcopt.thresslip = sBSlipThreshold->value();
    prcopt.thresdop = sBDopplerThreshold->value();
    //prcopt.armaxiter = sBARIter->value();
    prcopt.niter = sBNumIteration->value();
    prcopt.minfix = sBMinFixSats->value();
    prcopt.minholdsats = sBMinHoldSats->value();
    prcopt.mindropsats = sBMinDropSats->value();
    prcopt.syncsol = cBSyncSolution->currentIndex();
    prcopt.arfilter = cBARFilter->currentIndex();
    if (prcopt.mode == PMODE_MOVEB && cBBaselineConstrain->isChecked()) {
        prcopt.baseline[0] = sBBaselineLen->value();
        prcopt.baseline[1] = sBBaselineSig->value();
    }
    solopt.posf = cBSolutionFormat->currentIndex();
    solopt.timef = cBTimeFormat->currentIndex() == 0 ? 0 : 1;
    solopt.times = cBTimeFormat->currentIndex() == 0 ? 0 : cBTimeFormat->currentIndex() - 1;
    solopt.timeu = sBTimeDecimal->value();
    solopt.degf = cBLatLonFormat->currentIndex();
    strncpy(solopt.sep, qPrintable(FieldSep_Text), 63);
    solopt.outhead = cBOutputHeader->currentIndex();
    solopt.outopt = cBOutputOptions->currentIndex();
    solopt.outvel = cBOutputVelocity->currentIndex();
    prcopt.outsingle = cBOutputSingle->currentIndex();
    solopt.maxsolstd = sBMaxSolutionStd->value();
    solopt.datum = cBOutputDatum->currentIndex();
    solopt.height = cBOutputHeight->currentIndex();
    solopt.geoid = cBOutputGeoid->currentIndex();
    solopt.solstatic = cBSolutionFormat->currentIndex();
    solopt.nmeaintv[0] = sBNmeaInterval1->value();
    solopt.nmeaintv[1] = sBNmeaInterval2->value();
    solopt.trace = cBDebugTrace->currentIndex();
    solopt.sstat = cBDebugStatus->currentIndex();

    prcopt.eratio[0] = sBMeasurementErrorR1->value();
    prcopt.eratio[1] = sBMeasurementErrorR2->value();
    prcopt.eratio[2] = sBMeasurementErrorR5->value();
    prcopt.err[1] = sBMeasurementError2->value();
    prcopt.err[2] = sBMeasurementError3->value();
    prcopt.err[3] = sBMeasurementError4->value();
    prcopt.err[4] = sBMeasurementError5->value();
    prcopt.sclkstab = sBSatelliteClockStability->value();
    prcopt.prn[0] = sBProcessNoise1->value();
    prcopt.prn[1] = sBProcessNoise2->value();
    prcopt.prn[2] = sBProcessNoise3->value();
    prcopt.prn[3] = sBProcessNoise4->value();
    prcopt.prn[4] = sBProcessNoise5->value();

    if (cBRoverAntennaPcv->isChecked()) strncpy(prcopt.anttype[0], qPrintable(RovAnt_Text), 63);
    if (cBReferenceAntennaPcv->isChecked()) strncpy(prcopt.anttype[1], qPrintable(RefAnt_Text), 63);
    prcopt.antdel[0][0] = sBRoverAntennaE->value();
    prcopt.antdel[0][1] = sBRoverAntennaN->value();
    prcopt.antdel[0][2] = sBRoverAntennaU->value();
    prcopt.antdel[1][0] = sBReferenceAntennaE->value();
    prcopt.antdel[1][1] = sBReferenceAntennaN->value();
    prcopt.antdel[1][2] = sBReferenceAntennaU->value();

    prcopt.intpref = cBIntputReferenceObservation->currentIndex();
    prcopt.sbassatsel = lESbasSat->text().toInt();
    prcopt.rovpos = cBRoverPositionType->currentIndex() < 3 ? 0 : cBRoverPositionType->currentIndex() - 2;
    prcopt.refpos = cBReferencePositionType->currentIndex() < 3 ? 0 : cBReferencePositionType->currentIndex() - 2;
    if (prcopt.rovpos == POSOPT_POS) getPosition(cBRoverPositionType->currentIndex(), editu, prcopt.ru);
    if (prcopt.refpos == POSOPT_POS) getPosition(cBReferencePositionType->currentIndex(), editr, prcopt.rb);

    strncpy(prcopt.rnxopt[0], qPrintable(RnxOpts1_Text), 255);
    strncpy(prcopt.rnxopt[1], qPrintable(RnxOpts2_Text), 255);
    strncpy(prcopt.pppopt, qPrintable(PPPOpts_Text), 255);

    strncpy(filopt.satantp, qPrintable(SatPcvFile_Text), MAXSTRPATH-1);
    strncpy(filopt.rcvantp, qPrintable(AntPcvFile_Text), MAXSTRPATH-1);
    strncpy(filopt.stapos, qPrintable(lEStationPositionFile_Text), MAXSTRPATH-1);
    strncpy(filopt.geoid, qPrintable(GeoidDataFile_Text), MAXSTRPATH-1);
    strncpy(filopt.eop, qPrintable(EOPFile_Text), MAXSTRPATH-1);
    strncpy(filopt.dcb, qPrintable(DCBFile_Text), MAXSTRPATH-1);
    strncpy(filopt.blq, qPrintable(BLQFile_Text), MAXSTRPATH-1);
    strncpy(filopt.iono, qPrintable(IonoFile_Text), MAXSTRPATH-1);

    time2str(utc2gpst(timeget()), s, 0);
    sprintf(comment, qPrintable(tr("RTKPost_Qt options (%s, v.%s %s)")), s, VER_RTKLIB, PATCH_LEVEL);
    setsysopts(&prcopt, &solopt, &filopt);
    if (!saveopts(qPrintable(file), "w", comment, sysopts)) return;
}
//---------------------------------------------------------------------------
void OptDialog::updateEnable(void)
{
    bool rel = PMODE_DGPS <= cBPositionMode->currentIndex() && cBPositionMode->currentIndex() <= PMODE_FIXED;
    bool rtk = PMODE_KINEMA <= cBPositionMode->currentIndex() && cBPositionMode->currentIndex() <= PMODE_FIXED;
    bool ppp = cBPositionMode->currentIndex() >= PMODE_PPP_KINEMA;
    bool ar = rtk || ppp;

    cBFrequencies->setEnabled(rel || ppp);
    cBSolution->setEnabled(rel || ppp);
    cBDynamicModel->setEnabled(rel);
    cBTideCorrection->setEnabled(rel || ppp);
    //IonoOpt->setEnabled(!ppp);
    cBPositionOption1->setEnabled(ppp);
    cBPositionOption2->setEnabled(ppp);
    cBPositionOption3->setEnabled(ppp);
    cBPositionOption4->setEnabled(ppp);
    cBPositionOption5->setEnabled(ppp);
    cBPositionOption6->setEnabled(ppp);

    cBAmbiguityResolutionGPS->setEnabled(ar);
    cBAmbiguityResolutionGLO->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() > 0 && cBNavSys2->isChecked());
    cBAmbiguityResolutionBDS->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() > 0 && cBNavSys6->isChecked());
    sBValidThresAR->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() >= 1 && cBAmbiguityResolutionGPS->currentIndex() < 4);
    sBValidThresARMin->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() >= 1 && cBAmbiguityResolutionGPS->currentIndex() < 4);
    sBValidThresARMax->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() >= 1 && cBAmbiguityResolutionGPS->currentIndex() < 4);
    sBMaxPositionVarAR->setEnabled(ar && !ppp);
    sBGlonassHwBias->setEnabled(ar && cBAmbiguityResolutionGLO->currentIndex() == 2);
    sBLockCountFixAmbiguity->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() >= 1);
    sBElevationMaskAR->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() >= 1);
    sBOutageCountResetAmbiguity->setEnabled(ar || ppp);
    sBFixCountHoldAmbiguity->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() == 3);
    sBElevationMaskHold->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() == 3);
    sBSlipThreshold->setEnabled(ar || ppp);
    sBDopplerThreshold->setEnabled(ar || ppp);
    sBMaxAgeDifferences->setEnabled(rel);
    sBRejectCode->setEnabled(rel || ppp);
    sBRejectPhase->setEnabled(rel || ppp);
    sBVarHoldAmb->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() == 3);
    sBGainHoldAmb->setEnabled(ar && cBAmbiguityResolutionGPS->currentIndex() == 3);
    //sBARIter->setEnabled(ppp);
    sBMinFixSats->setEnabled(ar || ppp);
    sBMinHoldSats->setEnabled(ar || ppp);
    sBMinDropSats->setEnabled(ar || ppp);
    sBMaxPositionVarAR->setEnabled(ar || ppp);
    sBNumIteration->setEnabled(rel || ppp);
    cBARFilter->setEnabled(ar || ppp);

    cBBaselineConstrain->setEnabled(cBPositionMode->currentIndex() == PMODE_MOVEB);
    sBBaselineLen->setEnabled(cBBaselineConstrain->isChecked() && cBPositionMode->currentIndex() == PMODE_MOVEB);
    sBBaselineSig->setEnabled(cBBaselineConstrain->isChecked() && cBPositionMode->currentIndex() == PMODE_MOVEB);

    cBOutputHeader->setEnabled(cBSolutionFormat->currentIndex() < 3);
    cBOutputOptions->setEnabled(cBSolutionFormat->currentIndex() < 3);
    cBTimeFormat->setEnabled(cBSolutionFormat->currentIndex() < 3);
    sBTimeDecimal->setEnabled(cBSolutionFormat->currentIndex() < 3);
    cBLatLonFormat->setEnabled(cBSolutionFormat->currentIndex() == 0);
    lEFieldSeperator->setEnabled(cBSolutionFormat->currentIndex() < 3);
    cBOutputSingle->setEnabled(cBPositionMode->currentIndex() != 0);
    cBOutputDatum->setEnabled(cBSolutionFormat->currentIndex() == 0);
    cBOutputHeight->setEnabled(cBSolutionFormat->currentIndex() == 0);
    cBOutputGeoid->setEnabled(cBSolutionFormat->currentIndex() == 0 && cBOutputHeight->currentIndex() == 1);
    cBSolutionStatic->setEnabled(cBPositionMode->currentIndex() == PMODE_STATIC ||
                  cBPositionMode->currentIndex() == PMODE_PPP_STATIC);

    cBRoverAntennaPcv->setEnabled(rel || ppp);
    cBRoverAntenna->setEnabled((rel || ppp) && cBRoverAntennaPcv->isChecked());
    sBRoverAntennaE->setEnabled((rel || ppp) && cBRoverAntennaPcv->isChecked()&&cBRoverAntenna->currentText()!="*");
    sBRoverAntennaN->setEnabled((rel || ppp) && cBRoverAntennaPcv->isChecked()&&cBRoverAntenna->currentText()!="*");
    sBRoverAntennaU->setEnabled((rel || ppp) && cBRoverAntennaPcv->isChecked()&&cBRoverAntenna->currentText()!="*");
    lblRoverAntennaD->setEnabled((rel || ppp) && cBRoverAntennaPcv->isChecked()&&cBRoverAntenna->currentText()!="*");
    cBReferenceAntennaPcv->setEnabled(rel);
    cBReferenceAntenna->setEnabled(rel && cBReferenceAntennaPcv->isChecked());
    sBReferenceAntennaE->setEnabled(rel && cBReferenceAntennaPcv->isChecked()&&cBReferenceAntenna->currentText()!="*");
    sBReferenceAntennaN->setEnabled(rel && cBReferenceAntennaPcv->isChecked()&&cBReferenceAntenna->currentText()!="*");
    sBReferenceAntennaU->setEnabled(rel && cBReferenceAntennaPcv->isChecked()&&cBReferenceAntenna->currentText()!="*");
    lblReferenceAntennaD->setEnabled(rel && cBReferenceAntennaPcv->isChecked()&&cBReferenceAntenna->currentText()!="*");

    cBRoverPositionType->setEnabled(cBPositionMode->currentIndex() == PMODE_FIXED || cBPositionMode->currentIndex() == PMODE_PPP_FIXED);
    lERoverPosition1->setEnabled(cBRoverPositionType->isEnabled() && cBRoverPositionType->currentIndex() <= 2);
    lERoverPosition2->setEnabled(cBRoverPositionType->isEnabled() && cBRoverPositionType->currentIndex() <= 2);
    lERoverPosition3->setEnabled(cBRoverPositionType->isEnabled() && cBRoverPositionType->currentIndex() <= 2);
    btnRoverPosition->setEnabled(cBRoverPositionType->isEnabled() && cBRoverPositionType->currentIndex() <= 2);

    cBReferencePositionType->setEnabled(rel && cBPositionMode->currentIndex() != PMODE_MOVEB);
    lEReferencePosition1->setEnabled(cBReferencePositionType->isEnabled() && cBReferencePositionType->currentIndex() <= 2);
    lEReferencePosition2->setEnabled(cBReferencePositionType->isEnabled() && cBReferencePositionType->currentIndex() <= 2);
    lEReferencePosition3->setEnabled(cBReferencePositionType->isEnabled() && cBReferencePositionType->currentIndex() <= 2);
    btnReferencePosition->setEnabled(cBReferencePositionType->isEnabled() && cBReferencePositionType->currentIndex() <= 2);
}
//---------------------------------------------------------------------------
void OptDialog::getPosition(int type, QLineEdit **edit, double *pos)
{
    double p[3] = { 0 }, dms1[3] = { 0 }, dms2[3] = { 0 };

    if (type == 1) { /* lat/lon/height dms/m */
        QStringList tokens = edit[0]->text().split(" ");
        for (int i = 0; i < 3 || i < tokens.size(); i++)
            dms1[i] = tokens.at(i).toDouble();

        tokens = edit[1]->text().split(" ");
        for (int i = 0; i < 3 || i < tokens.size(); i++)
            dms2[i] = tokens.at(i).toDouble();

        p[0] = (dms1[0] < 0 ? -1 : 1) * (fabs(dms1[0]) + dms1[1] / 60 + dms1[2] / 3600) * D2R;
        p[1] = (dms2[0] < 0 ? -1 : 1) * (fabs(dms2[0]) + dms2[1] / 60 + dms2[2] / 3600) * D2R;
        p[2] = edit[2]->text().toDouble();
        pos2ecef(p, pos);
    } else if (type == 2) { /* x/y/z-ecef */
        pos[0] = edit[0]->text().toDouble();
        pos[1] = edit[1]->text().toDouble();
        pos[2] = edit[2]->text().toDouble();
    } else {   /* lat/lon/hight decimal */
        p[0] = edit[0]->text().toDouble() * D2R;
        p[1] = edit[1]->text().toDouble() * D2R;
        p[2] = edit[2]->text().toDouble();
        pos2ecef(p, pos);
    }
}
//---------------------------------------------------------------------------
void OptDialog::setPosition(int type, QLineEdit **edit, double *pos)
{
    double p[3], dms1[3], dms2[3], s1, s2;

    if (type == 1) { /* lat/lon/height dms/m */
        ecef2pos(pos, p);
        s1 = p[0] < 0 ? -1 : 1;
        s2 = p[1] < 0 ? -1 : 1;
        p[0] = fabs(p[0]) * R2D + 1E-12;
        p[1] = fabs(p[1]) * R2D + 1E-12;
        dms1[0] = floor(p[0]); p[0] = (p[0] - dms1[0]) * 60.0;
        dms1[1] = floor(p[0]); dms1[2] = (p[0] - dms1[1]) * 60.0;
        dms2[0] = floor(p[1]); p[1] = (p[1] - dms2[0]) * 60.0;
        dms2[1] = floor(p[1]); dms2[2] = (p[1] - dms2[1]) * 60.0;
        edit[0]->setText(QString("%1 %2 %3").arg(s1 * dms1[0], 0, 'f', 0).arg(dms1[1], 2, 'f', 0, '0').arg(dms1[2], 9, 'f', 6, '0'));
        edit[1]->setText(QString("%1 %2 %3").arg(s2 * dms2[0], 0, 'f', 0).arg(dms2[1], 2, 'f', 0, '0').arg(dms2[2], 9, 'f', 6, '0'));
        edit[2]->setText(QString::number(p[2], 'f', 4));
    } else if (type == 2) { /* x/y/z-ecef */
        edit[0]->setText(QString::number(pos[0], 'f', 4));
        edit[1]->setText(QString::number(pos[1], 'f', 4));
        edit[2]->setText(QString::number(pos[2], 'f', 4));
    } else {
        ecef2pos(pos, p);
        edit[0]->setText(QString::number(p[0] * R2D, 'f', 9));
        edit[1]->setText(QString::number(p[1] * R2D, 'f', 9));
        edit[2]->setText(QString::number(p[2], 'f', 4));
  }
}
//---------------------------------------------------------------------------
void OptDialog::readAntennaList(void)
{
    QString antennaPcvFile_Text = lEAntennaPcvFile->text();
    pcvs_t pcvs = { 0, 0, 0 };
    char *p;
    QString roverAntenna_Text, referenceAntenna_Text;
    int i;

    if (!readpcv(qPrintable(antennaPcvFile_Text), &pcvs)) return;

    /* Save currently defined antennas */
    roverAntenna_Text = cBRoverAntenna->currentText();
    referenceAntenna_Text = cBReferenceAntenna->currentText();

    /* Clear and add antennas from ANTEX file */
    cBRoverAntenna->clear();
    cBReferenceAntenna->clear();

    cBRoverAntenna->addItem(""); cBReferenceAntenna->addItem("");
    cBRoverAntenna->addItem("*"); cBReferenceAntenna->addItem("*");

    for (int i = 0; i < pcvs.n; i++) {
    if (pcvs.pcv[i].sat) continue;
        if ((p = strchr(pcvs.pcv[i].type, ' '))) *p = '\0';
        if (i > 0 && !strcmp(pcvs.pcv[i].type, pcvs.pcv[i - 1].type)) continue;
        cBRoverAntenna->addItem(pcvs.pcv[i].type);
        cBReferenceAntenna->addItem(pcvs.pcv[i].type);
    }

    /* Restore previously defined antennas */
    i = cBRoverAntenna->findText(roverAntenna_Text);
    cBRoverAntenna->setCurrentIndex(i == -1 ? 0 : i);
    i = cBReferenceAntenna->findText(referenceAntenna_Text);
    cBReferenceAntenna->setCurrentIndex(i == -1 ? 0 : i);

    free(pcvs.pcv);
}
//---------------------------------------------------------------------------
void OptDialog::showKeyDialog()
{
    KeyDialog *keyDialog = new KeyDialog(this);

    keyDialog->setFlag(2);
    keyDialog->exec();

    delete keyDialog;
}
//---------------------------------------------------------------------------
void OptDialog::showMaskDialog()
{
    MaskOptDialog *maskOptDialog = new MaskOptDialog(this);
    
    maskOptDialog->setSnrMask(snrMask);
    maskOptDialog->exec();
    if (maskOptDialog->result() != QDialog::Accepted) return;
    snrMask = maskOptDialog->getSnrMask();

    delete  maskOptDialog;
}
//---------------------------------------------------------------------------
void OptDialog::showFrequenciesDialog()
{
    FreqDialog *freqDialog = new FreqDialog(this);

    freqDialog->exec();

    delete freqDialog;
}
