#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A. Huck
 Date:          December 2018
________________________________________________________________________

-*/

#include "uiwebengine.h"
#include "i_common.h"

#include <QWebEngineView>


//! Helper class for uiWebEngine to relay Qt's messages.
/*!
    Internal object, to hide Qt's signal/slot mechanism.
*/

QT_BEGIN_NAMESPACE

class i_WebEngineViewMessenger : public QObject
{
    Q_OBJECT
    friend class	uiWebEngineViewBody;

protected:

i_WebEngineViewMessenger( QWebEngineView* sndr, uiWebEngineBase* receiver )
    : sender_(sndr)
    , receiver_(receiver)
{
 /*   connect( sndr, SIGNAL(back()), this, SLOT(back()) );
    connect( sndr, SIGNAL(forward()), this, SLOT(forward()) );
    connect( sndr, SIGNAL(reload()), this, SLOT(reload()) );
    connect( sndr, SIGNAL(stop()), this, SLOT(stop()) ); */
}

private:

    uiWebEngineBase*	receiver_;
    QWebEngineView*	sender_;
/*
private slots:

void back()
{ receiver_->backPressed.trigger( *receiver_ ); }

void forward()
{ receiver_->forwardPressed.trigger( *receiver_ ); }

void reload()
{ receiver_->reloadPressed.trigger( *receiver_ ); }

void stop()
{ receiver_->stopPressed.trigger( *receiver_ ); }
*/
};

QT_END_NAMESPACE
