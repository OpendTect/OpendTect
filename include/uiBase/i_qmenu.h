#ifndef i_qmenu_h
#define i_qmenu_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include <QObject>
#include <QAction>
#include <QMenuBar>
#include <QToolTip>

#include "uimenu.h"

//! Helper class for uiMenuItem to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
    Can only be constructed by uiMenuItem, which should connect 'activated' 
    slot to the corresponging QMenuItem when calling 'insertItem' on a 
    QMenuData object.
*/

QT_BEGIN_NAMESPACE

class i_MenuMessenger : public QObject 
{

    Q_OBJECT
    friend class                uiMenuItem;
    template<class> friend class uiMenuItemContainerBodyImpl;

protected:
i_MenuMessenger( QMenuBar* qmenubar )
    : receiver_(0)
    , qmenubar_(qmenubar)
{
    connect( qmenubar, SIGNAL(hovered(QAction*)), this, SLOT(hovered(QAction*)) );
}

i_MenuMessenger( uiMenuItem* receiver )
    : receiver_( receiver )
    , qmenubar_(0)
{}

private:

    uiMenuItem*		receiver_;
    QMenuBar*		qmenubar_;

private slots:

void activated()
{
    const int refnr = receiver_->beginCmdRecEvent();
    receiver_->activated.trigger( *receiver_ );
    receiver_->endCmdRecEvent( refnr );
}

void hovered( QAction* qaction )
{
    if ( !qmenubar_ ) return;

/*    QToolTip::showText( qmenubar_->geometry().bottomRight(),
			qaction->toolTip(), 0 ); */
}

};

QT_END_NAMESPACE

#endif
