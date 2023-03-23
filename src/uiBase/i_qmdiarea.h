#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimdiarea.h"

#include <QWidget>
#include <QMdiArea>
#include <QMdiSubWindow>


//! Helper class for uiMdiArea to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_MdiAreaMessenger : public QObject
{
Q_OBJECT
friend class uiMdiAreaBody;

protected:

i_MdiAreaMessenger( QMdiArea* sndr, uiMdiArea* rec )
    : sender_(sndr)
    , receiver_(rec)
{
    connect( sndr, &QMdiArea::subWindowActivated,
	     this, &i_MdiAreaMessenger::subWindowActivated );
}


~i_MdiAreaMessenger()
{}


private:

    QMdiArea*	sender_;
    uiMdiArea*	receiver_;

private slots:

void subWindowActivated( QMdiSubWindow* )
{
    receiver_->windowActivated.trigger( *receiver_ );
}

};

QT_END_NAMESPACE
