#ifndef i_qtabbar_h
#define i_qtabbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          14/02/2003
 RCS:           $Id: i_qtabbar.h,v 1.4 2003-11-07 12:21:53 bert Exp $
________________________________________________________________________

-*/

#include <uitabbar.h>

#include <qobject.h>
#include <qtabbar.h>

//! Helper class for uitabbar to relay Qt's 'currentChanged' messages to uiMenuItem.
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
			    connect( sender, SIGNAL( selected(int) ),
				     this,   SLOT( selected(int)) );
			}

    virtual		~i_tabbarMessenger() {}
   
private:

    uiTabBar*		_receiver;
    QTabBar*     	_sender;

private slots:

/*!
    Handler for currentChanged events.
    \sa QTabWidget::currentChanged
*/

    void		selected ( int id )
			{
			    _receiver->selected.trigger(*_receiver);
			}

};

#endif
