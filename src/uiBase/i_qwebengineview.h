#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A. Huck
 Date:          December 2018
________________________________________________________________________

-*/

#include "uiwebengine.h"

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
    connect( sndr, SIGNAL(loadFinished(bool)),
	     this, SLOT(loadFinished(bool)) );
}

private:

    uiWebEngineBase*	receiver_;
    QWebEngineView*	sender_;

private slots:

void loadFinished( bool res )
{ receiver_->loadFinished.trigger( res ); }

};

QT_END_NAMESPACE
