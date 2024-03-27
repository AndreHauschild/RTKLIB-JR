//---------------------------------------------------------------------------
#include <string.h>

#include "postmain.h"
#include "kmzconv.h"
#include "rtklib.h"
#include "viewer.h"

#include <QShowEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QtGlobal>
#include <QFileSystemModel>
#include <QCompleter>

extern MainForm *mainForm;

//---------------------------------------------------------------------------
ConvDialog::ConvDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    viewer = new TextViewer(this);

    QCompleter *fileCompleter = new QCompleter(this);
    QFileSystemModel *fileModel = new QFileSystemModel(fileCompleter);
    fileModel->setRootPath("");
    fileCompleter->setModel(fileModel);
    lEInputFile->setCompleter(fileCompleter);
    lEOutputFile->setCompleter(fileCompleter);
    lEGoogleEarthFile->setCompleter(fileCompleter);

    connect(btnClose, &QPushButton::clicked, this, &ConvDialog::accept);
    connect(btnConvert, &QPushButton::clicked, this, &ConvDialog::btnConvertClicked);
    connect(btnGoogleEarthFile, &QPushButton::clicked, this, &ConvDialog::btnGoogleEarthFileClicked);
    connect(btnView, &QPushButton::clicked, this, &ConvDialog::btnViewClicked);
    connect(btnGoogleEarth, &QPushButton::clicked, this, &ConvDialog::btnGoogleEarthClick);
    connect(cBAddOffset, &QCheckBox::clicked, this, &ConvDialog::updateEnable);
    connect(cBTimeSpan, &QCheckBox::clicked, this, &ConvDialog::updateEnable);
    connect(sBTimeInterval, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ConvDialog::updateEnable);
    connect(lEInputFile, &QLineEdit::editingFinished, this, &ConvDialog::updateEnable);
    connect(cBCompress, &QCheckBox::clicked, this, &ConvDialog::updateOutputFile);
    connect(lEGoogleEarthFile, &QLineEdit::editingFinished, this, &ConvDialog::googleEarthFileChanged);
    connect(btnInputFile, &QPushButton::clicked, this, &ConvDialog::btnInputFileClicked);
    connect(rBFormatKML, &QRadioButton::clicked, this, &ConvDialog::formatKMLClicked);
    connect(rBFormatGPX, &QRadioButton::clicked, this, &ConvDialog::formatGPXClicked);

	updateEnable();
}
//---------------------------------------------------------------------------
void ConvDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    rBFormatGPX->setChecked(!rBFormatKML->isChecked());
    lEGoogleEarthFile->setText(mainForm->googleEarthFile);
}
//---------------------------------------------------------------------------
void ConvDialog::setInput(const QString &File)
{
    lEInputFile->setText(File);
	updateOutputFile();
}

//---------------------------------------------------------------------------
void ConvDialog::btnInputFileClicked()
{
    lEInputFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open..."), lEInputFile->text(), tr("All (*.*)"))));
}
//---------------------------------------------------------------------------
void ConvDialog::btnConvertClicked()
{
    QString InputFile_Text = lEInputFile->text(), OutputFile_Text = lEOutputFile->text();
	int stat;
    QString cmd;
    QStringList opt;
    char file[1024], kmlfile[1024], *p;
    double offset[3] = { 0 }, tint = 0.0;
    gtime_t ts = { 0, 0 }, te = { 0, 0 };

	showMessage("");

    if (lEInputFile->text() == "" || lEOutputFile->text() == "") return;

    showMessage(tr("converting ..."));

    if (cBTimeSpan->isChecked()) {
        QDateTime start(dateTimeStart->dateTime());
        QDateTime end(dateTimeStop->dateTime());
        ts.time = start.toSecsSinceEpoch(); ts.sec = start.time().msec() / 1000;
        te.time = end.toSecsSinceEpoch(); te.sec = end.time().msec() / 1000;
  }

    if (cBAddOffset->isChecked()) {
        offset[0] = sBOffset1->value();
        offset[1] = sBOffset2->value();
        offset[2] = sBOffset3->value();
	}

    if (cBTimeInterval->isChecked()) tint = sBTimeInterval->value();

    strncpy(file, qPrintable(InputFile_Text), 1024);

    if (rBFormatKML->isChecked()) {
        if (cBCompress->isChecked()) {
            strncpy(kmlfile, file, 1020);
            if (!(p = strrchr(kmlfile, '.'))) p = kmlfile + strlen(kmlfile);
            strncpy(p, ".kml", 5);
        }
        stat = convkml(file, cBCompress->isChecked() ? kmlfile : qPrintable(OutputFile_Text),
                   ts, te, tint, cBQFlags->currentIndex(), offset,
                   cBTrackColor->currentIndex(), cBPointColor->currentIndex(),
                   cBOutputAltitude->currentIndex(), cBOutputTime->currentIndex());
    } else {
        stat = convgpx(file, cBCompress->isChecked() ? kmlfile : qPrintable(OutputFile_Text),
                   ts, te, tint, cBQFlags->currentIndex(), offset,
                   cBTrackColor->currentIndex(), cBPointColor->currentIndex(),
                   cBOutputAltitude->currentIndex(), cBOutputTime->currentIndex());
    }

    if (stat < 0) {
        if (stat == -1) showMessage(tr("error : read input file"));
        else if (stat == -2) showMessage(tr("error : input file format"));
        else if (stat == -3) showMessage(tr("error : no data in input file"));
        else showMessage(tr("error : write kml file"));
		return;
	}
    if (rBFormatKML->isChecked() && cBCompress->isChecked()) {
#ifdef Q_OS_WIN
        cmd = "zip.exe";
        opt << "-j"
        opt << QString("-m %1").arg(lEOutputFile->text()); //TODO: zip for other platforms
#endif
#ifdef Q_OS_LINUX
        cmd = "gzip";
        opt << QString("-3 %1 %2").arg(lEOutputFile->text());
#endif
        opt << kmlfile;
        if (!execCommand(cmd, opt)) {
            showMessage(tr("error : zip execution"));
			return;
		}
	}
	showMessage("done");
}

//---------------------------------------------------------------------------
void ConvDialog::updateEnable()
{
    sBOffset1->setEnabled(cBAddOffset->isChecked());
    sBOffset2->setEnabled(cBAddOffset->isChecked());
    sBOffset3->setEnabled(cBAddOffset->isChecked());
    dateTimeStart->setEnabled(cBTimeSpan->isChecked());
    dateTimeStop->setEnabled(cBTimeSpan->isChecked());
    sBTimeInterval->setEnabled(cBTimeInterval->isChecked());

    cBCompress->setVisible(rBFormatKML->isChecked());
    lEGoogleEarthFile->setEnabled(rBFormatKML->isChecked());
    btnGoogleEarthFile->setEnabled(rBFormatKML->isChecked());
    btnGoogleEarth->setEnabled(rBFormatKML->isChecked());

    cBCompress->setEnabled(false);
#ifdef Q_OS_WIN
    cBCompress->setEnabled(true);
#endif
#ifdef Q_OS_LINUX
    cBCompress->setEnabled(true);
#endif
}
//---------------------------------------------------------------------------
int ConvDialog::execCommand(const QString &cmd, const QStringList &opt)
{
    return QProcess::startDetached(cmd, opt);
}
//---------------------------------------------------------------------------
void ConvDialog::showMessage(const QString &msg)
{
    lblMessage->setText(msg);
    if (msg.toLower().contains("error")) lblMessage->setStyleSheet("QLabel {color : red}");
    else lblMessage->setStyleSheet("QLabel {color : blue}");
}
//---------------------------------------------------------------------------
void ConvDialog::updateOutputFile()
{
    QString inputFile_Text = lEInputFile->text();

    if (inputFile_Text == "") return;

    QFileInfo fi(inputFile_Text);
    if (fi.suffix().isNull()) return;

    inputFile_Text = QDir::toNativeSeparators(fi.absolutePath() + QDir::separator() + fi.baseName());
    inputFile_Text += rBFormatGPX->isChecked() ? ".gpx" : (cBCompress->isChecked() ? ".kmz" : ".kml");
    lEOutputFile->setText(inputFile_Text);
}
//---------------------------------------------------------------------------
void ConvDialog::googleEarthFileChanged()
{
    mainForm->googleEarthFile = lEGoogleEarthFile->text();
}
//---------------------------------------------------------------------------
void ConvDialog::btnGoogleEarthFileClicked()
{
    lEGoogleEarthFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Google Earth Exe File"), lEGoogleEarthFile->text())));
}
//---------------------------------------------------------------------------
void ConvDialog::btnGoogleEarthClick()
{
    QString cmd;
    QStringList opt;

    cmd = lEGoogleEarthFile->text();
    opt << lEOutputFile->text();
    
    if (!execCommand(cmd, opt))
        showMessage(tr("error : Google Earth execution"));

    return;
}
void ConvDialog::formatKMLClicked()
{
    updateOutputFile();
    updateEnable();
}
//---------------------------------------------------------------------------
void ConvDialog::formatGPXClicked()
{
    updateOutputFile();
    updateEnable();
}
//---------------------------------------------------------------------------
void ConvDialog::btnViewClicked()
{
    QString file = lEOutputFile->text();

    if (file == "") return;

    viewer->setWindowTitle(file);
    viewer->show();
    viewer->read(file);
}
//---------------------------------------------------------------------------
