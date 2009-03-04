#ifndef i_qmenu_h
#define i_qmenu_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: i_qmenu.h,v 1.9 2009-03-04 10:43:27 cvsjaap Exp $
________________________________________________________________________

-*/

#include <QObject>
#include <QEvent>
#include "uimenu.h"

//! Helper class for uiMenuItem to relay Qt's 'activated' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
    Can only be constructed by uiMenuItem, which should connect 'activated' 
    slot to the corresponging QMenuItem when calling 'insertItem' on a 
    QMenuData object.
*/
class i_MenuMessenger : public QObject 
{

    Q_OBJECT
    friend class                uiMenuItem;

protected:
				i_MenuMessenger( uiMenuItem* receiver )
                                : _receiver( receiver )
				{}

    bool			event( QEvent* ev ) 
				{ return _receiver->handleEvent(ev) 
						? true : QObject::event(ev); }

private:

    uiMenuItem*			_receiver;

private slots:

    void			activated()
    				{
				    _receiver->markCmdRecEvent( true );
				    _receiver->activated.trigger( *_receiver );
				    _receiver->markCmdRecEvent( false );
			       	}

};

#endif
