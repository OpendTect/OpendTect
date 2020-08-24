#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          November 2012
________________________________________________________________________

-*/

#include "notify.h"
#include <QObject>

QT_BEGIN_NAMESPACE

/*!
\brief Helper class for QPtr to relay Qt's messages. Internal object, to hide
Qt's signal/slot mechanism.
*/

class i_QPtrImpl : public QObject, public CallBacker
{
    Q_OBJECT

public:

    Notifier<i_QPtrImpl> notifier_;
    Threads::Lock	lock_;

    QObject*		ptr()				{ return sender_; }
    const QObject*	ptr() const			{ return sender_; }


			operator QObject*()		{ return sender_; }
			operator const QObject*() const	{ return sender_; }
    QObject*		operator->()			{ return sender_; }
    QObject*		operator->() const		{ return sender_; }
    QObject*		operator=(QObject* qo)		{ set( qo ); return qo;}

    Threads::Lock&	objLock()			{ return lock_; }


    void		set(QObject* qo);
			i_QPtrImpl( QObject* sndr = 0 );
			~i_QPtrImpl();

private:

    QObject*		sender_;

private slots:

    void		destroyed( QObject* )
			{
			    notifier_.trigger();
			    Threads::Locker lckr( lock_ );
			    sender_ = 0;
			}

};

QT_END_NAMESPACE
