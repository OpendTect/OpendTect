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

i_MdiAreaMessenger( QMdiArea* sndr, uiMdiArea* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
    connect( sndr, SIGNAL(subWindowActivated(QMdiSubWindow*)),
	     this, SLOT(subWindowActivated(QMdiSubWindow*)) );
}

private:

    uiMdiArea*	receiver_;
    QMdiArea*	sender_;

private slots:

void subWindowActivated( QMdiSubWindow* )
{ receiver_->windowActivated.trigger( *receiver_ ); }

};

QT_END_NAMESPACE
