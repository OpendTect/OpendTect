#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidialog.h"

#include <QObject>
#include <QDialog> 

QT_BEGIN_NAMESPACE

class QDialogMessenger : public QObject 
{
Q_OBJECT
friend class uiDialog;

protected:

QDialogMessenger( QDialog* qdlg, uiDialog* uidlg )
    : qdialog_(qdlg)
    , uidialog_(uidlg)
{
   connect( qdialog_, &QDialog::accepted, this, &QDialogMessenger::accepted );
   connect( qdialog_, &QDialog::rejected, this, &QDialogMessenger::rejected );
   connect( qdialog_, &QDialog::finished, this, &QDialogMessenger::finished );
}


~QDialogMessenger()
{}


private slots:

void accepted()
{ // Implement when uiDialog uses QDialog again
}

void rejected()
{ // Implement when uiDialog uses QDialog again
}

void finished( int result )
{ // Implement when uiDialog uses QDialog again
}

private:

    QDialog*		qdialog_;
    uiDialog*		uidialog_;
};

QT_END_NAMESPACE
