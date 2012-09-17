#ifndef i_qmdiarea_h
#define i_qmdiarea_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2007
 RCS:           $Id: i_qmdiarea.h,v 1.3 2011/04/21 13:09:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimdiarea.h"

#include <QObject>
#include <QWidget>
#include <QMdiArea> 


//! Helper class for uiMdiArea to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/


class i_MdiAreaMessenger : public QObject 
{
    Q_OBJECT
    friend class uiMdiAreaBody;

protected:

i_MdiAreaMessenger( QMdiArea* sndr, uiMdiArea* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( sndr, SIGNAL(subWindowActivated(QMdiSubWindow*)),
	     this, SLOT(subWindowActivated(QMdiSubWindow*)) );
}

private:

    uiMdiArea*	receiver_;
    QMdiArea*  	sender_;

private slots:

void subWindowActivated( QMdiSubWindow* )
{ receiver_->windowActivated.trigger( *receiver_ ); }

};

#endif
