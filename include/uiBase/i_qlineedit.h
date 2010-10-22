#ifndef i_qlineedit_h
#define i_qlineedit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: i_qlineedit.h,v 1.8 2010-10-22 15:22:22 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uilineedit.h"

#include <QLineEdit> 

class QString;

//! Helper class for uilineedit to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/
class i_lineEditMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiLineEditBody;

protected:
i_lineEditMessenger( QLineEdit* sender, uiLineEdit* receiver )
    : sender_( sender )
    , receiver_( receiver )
{ 
    connect( sender, SIGNAL(returnPressed()),
	     this, SLOT(returnPressed()) );
    connect( sender, SIGNAL(editingFinished()),
	     this, SLOT(editingFinished()) );
    connect( sender, SIGNAL(textChanged(const QString&)),
	     this, SLOT(textChanged(const QString&)) );
    connect( sender, SIGNAL(selectionChanged()),
	     this, SLOT(selectionChanged()) );
}

private:

    uiLineEdit* 	receiver_;
    QLineEdit*  	sender_;

private slots:

#define mTrigger( notifier ) \
    const int refnr = receiver_->beginCmdRecEvent( #notifier ); \
    receiver_->notifier.trigger(*receiver_); \
    receiver_->endCmdRecEvent( refnr, #notifier );

    void 		editingFinished()
			{
			    if ( !sender_->isModified() )
				return;

			    mTrigger( editingFinished );
			}

    void 		returnPressed()
			{ mTrigger( returnPressed ); }
    void 		textChanged(const QString&)
			{ mTrigger( textChanged ); }

    void		selectionChanged()
    			{ receiver_->selectionChanged.trigger(*receiver_); }
};

#endif
