#ifndef i_qlineedit_h
#define i_qlineedit_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: i_qlineedit.h,v 1.4 2008-01-08 03:45:00 cvsnanne Exp $
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

    void 		returnPressed() 
			{ receiver_->returnPressed.trigger(*receiver_); }
    void 		editingFinished() 
			{ receiver_->editingFinished.trigger(*receiver_); }
    void 		textChanged(const QString&)
			{ receiver_->textChanged.trigger(*receiver_); }
};

#endif
