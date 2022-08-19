#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitabbar.h"

#include <QTabBar>


//! Helper class for uitabbar to relay Qt's 'currentChanged' messages to
// uiAction.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_tabbarMessenger : public QObject
{
    Q_OBJECT
    friend class	uiTabBarBody;

protected:
i_tabbarMessenger( QTabBar* sndr, uiTabBar* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( sndr, SIGNAL(currentChanged(int)), this, SLOT(selected(int)) );
    connect( sndr, SIGNAL(tabCloseRequested(int)),
	     this, SLOT(tabToBeClosed(int)) );
}

private:

    uiTabBar*		receiver_;
    QTabBar*		sender_;

private slots:

    void		selected ( int id )
			{
			    const int refnr = receiver_->beginCmdRecEvent();
			    receiver_->selected.trigger(*receiver_);
			    receiver_->endCmdRecEvent( refnr );
			}

    void		tabToBeClosed( int tabidx )
			{
			    const int refnr = receiver_->beginCmdRecEvent();
			    receiver_->tabToBeClosed.trigger(tabidx,
							      *receiver_);
			    receiver_->endCmdRecEvent( refnr );
			}

};

QT_END_NAMESPACE
