//---------------------------------------------------------------------------

#include <QShowEvent>

#include "viewer.h"
#include "mapviewopt.h"

//---------------------------------------------------------------------------
MapViewOptDialog::MapViewOptDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);

    connect(btnOk, &QPushButton::clicked, this, &MapViewOptDialog::btnOkClicked);
    connect(btnCancel, &QPushButton::clicked, this, &MapViewOptDialog::close);
    connect(btnNotes, &QPushButton::clicked, this, &MapViewOptDialog::btnNotesClicked);
}
//---------------------------------------------------------------------------
void MapViewOptDialog::showEvent(QShowEvent *)
{
    QLineEdit *titles[]={
        lEMapTitle1, lEMapTitle2, lEMapTitle3, lEMapTitle4, lEMapTitle5, lEMapTitle6
	};
    QLineEdit *tiles[]={
        lEMapTile1, lEMapTile2, lEMapTile3, lEMapTile4, lEMapTile5, lEMapTile6
	};
	for (int i=0;i<6;i++) {
        titles[i]->setText(mapStrings[i][0]);
        tiles [i]->setText(mapStrings[i][1]);
	}
    lEApiKey->setText(apiKey);
}
//---------------------------------------------------------------------------
void MapViewOptDialog::btnOkClicked()
{
    QLineEdit *titles[]={
        lEMapTitle1, lEMapTitle2, lEMapTitle3, lEMapTitle4, lEMapTitle5, lEMapTitle6
	};
    QLineEdit *tiles[]={
        lEMapTile1, lEMapTile2, lEMapTile3, lEMapTile4, lEMapTile5, lEMapTile6
	};
	for (int i=0;i<6;i++) {
        mapStrings[i][0]=titles[i]->text();
        mapStrings[i][1]=tiles [i]->text();
	}
    apiKey=lEApiKey->text();
    accept();
}
//---------------------------------------------------------------------------
void MapViewOptDialog::btnNotesClicked()
{
    QString file, dir;
    TextViewer *viewer;
    
    dir = qApp->applicationDirPath(); // exe directory
    file=dir+"/gmview_notes.txt";
    viewer=new TextViewer(this);
    viewer->setWindowTitle(file);
    viewer->option=0;
    viewer->exec();
    viewer->read(file);	
}
//---------------------------------------------------------------------------


