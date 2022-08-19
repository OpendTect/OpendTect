#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "callback.h"
#include "threadlock.h"
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
