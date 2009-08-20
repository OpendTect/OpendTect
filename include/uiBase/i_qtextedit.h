#ifndef i_qtextedit_h
#define i_qtextedit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2007
 RCS:           $Id: i_qtextedit.h,v 1.1 2009-08-20 07:01:20 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uitextedit.h"

#include <QTextEdit>


//! Helper class for uiTextEdit to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/


class i_TextEditMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiTextEditBody;

protected:

i_TextEditMessenger( QTextEdit* sender, uiTextEdit* receiver )
    : sender_(sender)
    , receiver_(receiver)
{
    connect( sender, SIGNAL(textChanged()), this, SLOT(textChanged()) );
}

private:

    uiTextEdit*		receiver_;
    QTextEdit*  	sender_;

private slots:

void textChanged()
{ receiver_->textChanged.trigger( *receiver_ ); }

};

#endif
