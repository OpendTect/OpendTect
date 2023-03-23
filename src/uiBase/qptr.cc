/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "qptr.h"
#include "i_qptr.h"

// QObjPtr
QObjPtr::QObjPtr( QObject* qobj )
    : impl_(new i_QPtrImpl(qobj))
{}


QObjPtr::~QObjPtr()
{
    delete impl_;
}


QObjPtr::operator QObject*()
{
    return impl_->ptr();
}


QObjPtr::operator const QObject*() const
{
    return impl_->ptr();
}


const QObject* QObjPtr::operator->() const
{
    return impl_->ptr();
}


QObject* QObjPtr::operator->()
{
    return impl_->ptr();
}


QObject* QObjPtr::operator=( QObject* qo )
{
    impl_->set( qo );
    return qo; 
}


Threads::Mutex& QObjPtr::mutex()
{
    return impl_->lock_.mutex();
}


NotifierAccess& QObjPtr::deleteNotifier()
{
    return impl_->notifier_;
}


// i_QPtrImpl
i_QPtrImpl::i_QPtrImpl( QObject* sndr )
    : sender_(0)
    , notifier_(this)
{
    Threads::Locker lock( lock_ );
    set( sndr );
}


i_QPtrImpl::~i_QPtrImpl()
{
    Threads::Locker lock( lock_ );
    set( nullptr );
}

void i_QPtrImpl::set( QObject* qo )
{
    if ( sender_ )
	sender_->disconnect( this );

    sender_ = qo;
    if ( sender_ )
    {
	connect( sender_, &QObject::destroyed,
		 this, &i_QPtrImpl::_destroyed );
    }
}
