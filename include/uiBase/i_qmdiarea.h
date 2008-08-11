#ifndef i_qmdiarea_h
#define i_qmdiarea_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2007
 RCS:           $Id: i_qmdiarea.h,v 1.1 2008-08-11 18:22:17 cvsnanne Exp $
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

i_MdiAreaMessenger( QMdiArea* sender, uiMdiArea* receiver )
    : sender_(sender)
    , receiver_(receiver)
{
    connect( sender, SIGNAL(subWindowActivated(QMdiSubWindow*)),
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
