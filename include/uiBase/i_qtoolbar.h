#ifndef i_qtoolbar_h
#define i_qtoolbar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include <QToolBar>
#include "uitoolbar.h"


//! Helper class for uiToolBar to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_ToolBarMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiToolBar;

protected:
i_ToolBarMessenger( QToolBar* sndr, uiToolBar* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{ 
    connect( sndr, SIGNAL(actionTriggered(QAction*)),
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

QT_END_NAMESPACE

#endif
