#ifndef i_qtoolbar_h
#define i_qtoolbar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
 RCS:           $Id: i_qtoolbar.h,v 1.1 2010-01-28 05:34:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include <QToolBar>
#include "uitoolbar.h"


//! Helper class for uiToolBar to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

class i_ToolBarMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiToolBarBody;

protected:
i_ToolBarMessenger( QToolBar* sender, uiToolBar* receiver )
    : sender_(sender)
    , receiver_(receiver)
{ 
    connect( sender, SIGNAL(actionTriggered(QAction*)),
	     this, SLOT(actionTriggered(QAction*)) );
}

private:
    uiToolBar*		receiver_;
    QToolBar*		sender_;

private slots:

void actionTriggered( QAction* qaction )
{
    const int butid = receiver_->getButtonID( qaction );
    receiver_->buttonClicked.trigger( butid, *receiver_ );
}

};

#endif
