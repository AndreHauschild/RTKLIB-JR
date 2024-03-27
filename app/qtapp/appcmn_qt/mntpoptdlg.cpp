//---------------------------------------------------------------------------
#include <stdio.h>

#include <QShowEvent>

#include "rtklib.h"
#include "mntpoptdlg.h"
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
MntpOptDialog::MntpOptDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);

    connect(btnOk, &QPushButton::clicked, this, &MntpOptDialog::btnOkClicked);
    connect(btnCancel, &QPushButton::clicked, this, &MntpOptDialog::reject);
}
//---------------------------------------------------------------------------
void MntpOptDialog::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) return;

    QLineEdit *edit[]={NULL, NULL,
        lESourceTable3, lESourceTable4, NULL, NULL, lESourceTable7,
        lESourceTable8, lESourceTable9, NULL, NULL,
        NULL, NULL, lESourceTable14, lESourceTable15, NULL, NULL, NULL
    };
    QComboBox *box[]={NULL, NULL,
        NULL, NULL, cBSourceTable5, cBSourceTable6, NULL, NULL, NULL, NULL, NULL, cBSourceTable12,
        cBSourceTable13, NULL, NULL, cBSourceTable16, cBSourceTable17, NULL
    };
    QDoubleSpinBox *spinbox[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, sBSourceTable10,
        sBSourceTable11, NULL, NULL, NULL, NULL, NULL, NULL, sBSourceTable18
    };
    
    lEMountPoint->setText(mountPoint);

    QStringList tokens = mountpointString.split(";");

    for (int i = 3; i < 18; i++) {
        if (edit[i] != NULL) edit[i]->setText("");
        else if (spinbox[i] != NULL) spinbox[i]->setValue(0);
        else if (box[i] != NULL) {box[i]->setCurrentIndex(0);box[i]->setDisabled(true);}
    }

    for (int i = 3; i < tokens.size() && i < 18; i++) {
        if (edit[i] != NULL) edit[i]->setText(tokens.at(i));
        else if (spinbox[i] != NULL) spinbox[i]->setValue(tokens.at(i).toDouble());
        else if (box[i] != NULL) box[i]->setCurrentText(tokens.at(i));
    }
}
//---------------------------------------------------------------------------
void MntpOptDialog::btnOkClicked()
{
    mountPoint = lEMountPoint->text();
    mountpointString = "STR;"+mountPoint+lESourceTable3->text()+";"+lESourceTable4->text()+";"+QString::number(cBSourceTable5->currentIndex())+";"+
                       QString::number(cBSourceTable6->currentIndex())+";"+lESourceTable7->text()+";"+lESourceTable8->text()+";"+lESourceTable9->text()+";"+
                       QString::number(sBSourceTable10->value())+";"+QString::number(sBSourceTable11->value())+";"+
                       QString::number(cBSourceTable12->currentIndex())+";"+QString::number(cBSourceTable13->currentIndex())+";"+lESourceTable14->text()+";"+lESourceTable15->text()+";"+
                       QString::number(cBSourceTable16->currentIndex())+";"+QString::number(cBSourceTable17->currentIndex())+";"+QString::number(sBSourceTable18->value());

    accept();
}
//---------------------------------------------------------------------------

