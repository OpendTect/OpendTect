#ifndef i_qtabbar_h
#define i_qtabbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/01/2002
 RCS:           $Id: i_qtabbar.h,v 1.1 2002-01-16 11:01:23 arend Exp $
________________________________________________________________________

-*/

#include <uitabbar.h>

#include <qobject.h>
#include <qtabbar.h> 

//! Helper class for uiTabBar to relay Qt's 'selected' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_tabbarMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiTabBarBody;

protected:
			i_tabbarMessenger( QTabBar*  sender,
					   uiTabBar* receiver )
			: _sender( sender )
			, _receiver( receiver )
			{ 
			    connect( sender, SIGNAL( selected (int)),
				     this,   SLOT( selected (int)) );
			}

    virtual		~i_tabbarMessenger() {}
   
private:

    uiTabBar* 	_receiver;
    QTabBar*  	_sender;

private slots:

/*!
    Handler for selected events.
    \sa QTabBar::selected
*/

    void 		selected( int id ) 
			{_receiver->selected.trigger(*_receiver);}

};

#endif
