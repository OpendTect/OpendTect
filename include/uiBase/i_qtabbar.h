#ifndef i_qtabbar_h
#define i_qtabbar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          14/02/2003
 RCS:           $Id: i_qtabbar.h,v 1.10 2009-07-22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "uitabbar.h"

#include <QObject>
#include <QTabBar>


//! Helper class for uitabbar to relay Qt's 'currentChanged' messages to uiMenuItem.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_tabbarMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiTabBarBody;

protected:
i_tabbarMessenger( QTabBar* sender, uiTabBar* receiver )
    : sender_(sender)
    , receiver_(receiver)
{ 
    connect( sender, SIGNAL(currentChanged(int)), this, SLOT(selected(int)) );
}

private:

    uiTabBar*		receiver_;
    QTabBar*     	sender_;

private slots:

    void		selected ( int id )
			{
			    const int refnr = receiver_->beginCmdRecEvent();
			    receiver_->selected.trigger(*receiver_);
			    receiver_->endCmdRecEvent( refnr );
			}

};

#endif
