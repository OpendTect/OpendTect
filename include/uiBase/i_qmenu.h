#ifndef i_qmenu_h
#define i_qmenu_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: i_qmenu.h,v 1.11 2009-07-22 16:01:20 cvsbert Exp $
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
				    const int refnr =
						_receiver->beginCmdRecEvent();
				    _receiver->activated.trigger( *_receiver );
				    _receiver->endCmdRecEvent( refnr );
			       	}
};

#endif
