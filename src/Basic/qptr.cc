/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Nov 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "qptr.h"

#ifndef OD_NO_QT
#include "i_qptr.h"
#endif

QObjPtr::QObjPtr( QObject* qo )
#ifndef OD_NO_QT
    : impl_( new i_QPtrImpl( qo ) )
#else
    : impl_( 0 )
#endif
{}


QObjPtr::~QObjPtr()
{
#ifndef OD_NO_QT
    delete impl_;
#endif
}


QObjPtr::operator QObject*()
{
#ifndef OD_NO_QT
    return impl_->ptr();
#else
    return 0;
#endif
}


QObjPtr::operator const QObject*() const
{
#ifndef OD_NO_QT
    return impl_->ptr();
#else
    return 0;
#endif
}


const QObject* QObjPtr::operator->() const
{
#ifndef OD_NO_QT
    return impl_->ptr();
#else
    return 0;
#endif
}

QObject* QObjPtr::operator->()
{
#ifndef OD_NO_QT
    return impl_->ptr();
#else
    return 0;
#endif
}


QObject* QObjPtr::operator=( QObject* qo )
{
#ifndef OD_NO_QT
    impl_->set( qo );
    return qo;
#else
    return 0;
#endif
}

#ifndef OD_NO_QT
Threads::Mutex& QObjPtr::mutex()
{
    return impl_->lock_;
}


NotifierAccess& QObjPtr::deleteNotifier()
{
    return impl_->notifier_;
}

void i_QPtrImpl::set(QObject* qo )
{
    if ( sender_ ) sender_->disconnect( this );

    sender_ = qo;
    if ( sender_ )
    {
	connect( sender_, SIGNAL(destroyed(QObject*)),
		 this, SLOT(destroyed(QObject*)) );
    }
}

i_QPtrImpl::i_QPtrImpl( QObject* sndr )
    : sender_(0)
    , notifier_(this)
{
    Threads::MutexLocker lock( lock_ );
    set( sndr );
}

i_QPtrImpl::~i_QPtrImpl()
{
    Threads::MutexLocker lock( lock_ );
    set( 0 );
}

#endif
