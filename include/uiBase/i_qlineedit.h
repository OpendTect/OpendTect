#ifndef i_qlineedit_h
#define i_qlineedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: i_qlineedit.h,v 1.5 2009-05-15 16:27:46 cvsjaap Exp $
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
}

private:

    uiLineEdit* 	receiver_;
    QLineEdit*  	sender_;

private slots:

#define mTrigger( notifier ) \
    const int refnr = receiver_->beginCmdRecEvent( #notifier ); \
    receiver_->notifier.trigger(*receiver_); \
    receiver_->endCmdRecEvent( refnr, #notifier );

    void 		returnPressed()
			{ mTrigger( returnPressed ); }
    void 		editingFinished()
			{ mTrigger( editingFinished ); }
    void 		textChanged(const QString&)
			{ mTrigger( textChanged ); }
};

#endif
