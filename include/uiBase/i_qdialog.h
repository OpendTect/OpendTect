#ifndef i_qdialog_h
#define i_qdialog_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: i_qdialog.h,v 1.7 2012-08-28 09:26:42 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

#include <QObject>
#include <QDialog> 

mStartODQtNamespace

class QDialogMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiDialog;

protected:

QDialogMessenger( QDialog* qdlg, uiDialog* uidlg )
    : qdialog_(qdlg)
    , uidialog_(uidlg)
{
   connect( qdialog_, SIGNAL(accepted()), this, SLOT(accepted()) );
   connect( qdialog_, SIGNAL(rejected()), this, SLOT(rejected()) );
   connect( qdialog_, SIGNAL(finished(int)), this, SLOT(finished(int)) );
}


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

mStopODQtNamespace

#endif
