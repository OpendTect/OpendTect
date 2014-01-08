#ifndef i_qtextedit_h
#define i_qtextedit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitextedit.h"

#include <QTextEdit>


//! Helper class for uiTextEdit to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_TextEditMessenger : public QObject 
{
    Q_OBJECT
    friend class	uiTextEditBody;

protected:

i_TextEditMessenger( QTextEdit* sndr, uiTextEdit* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( sndr, SIGNAL(textChanged()), this, SLOT(textChanged()) );
}

private:

    uiTextEdit*		receiver_;
    QTextEdit*  	sender_;

private slots:

void textChanged()
{ receiver_->textChanged.trigger( *receiver_ ); }

};

QT_END_NAMESPACE

#endif
