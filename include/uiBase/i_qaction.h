#ifndef i_qaction_h
#define i_qaction_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2007
 RCS:           $Id: i_qaction.h,v 1.1 2007-08-08 05:22:41 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiaction.h"

#include <QObject>
#include <QAction>


//! Helper class for uiAction to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_ActionMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiActionBody;

protected:
			i_ActionMessenger( QAction*  sender,
					   uiAction* receiver )
			: sender_( sender )
			, receiver_( receiver )
			{ 
			    connect( sender, SIGNAL(toggled(bool)),
				     this, SLOT(toggled(bool)) );
			    connect( sender, SIGNAL(triggered(bool)),
				     this, SLOT(triggered(bool)));
			}

    virtual		~i_ActionMessenger() {}
   
private:

    uiAction*		receiver_;
    QAction*		sender_;

private slots:

    void 		toggled( bool checked )
			{
			    receiver_->checked_ = checked;
			    receiver_->toggled.trigger( *receiver_ );
			}

    void 		triggered( bool checked )
			{
			    receiver_->checked_ = checked;
			    receiver_->triggered.trigger( *receiver_ );
			}
};

#endif
