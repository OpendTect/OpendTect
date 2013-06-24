/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Nov 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "qptr.h"

#include "i_qptr.h"

QObjPtr::QObjPtr( QObject* qo )
    : impl_( new i_QPtrImpl( qo ) )
{}


QObjPtr::~QObjPtr()
{ delete impl_; }


QObjPtr::operator QObject*()
{ return impl_->ptr(); }


QObjPtr::operator const QObject*() const
{ return impl_->ptr(); }


const QObject* QObjPtr::operator->() const
{ return impl_->ptr(); }

QObject* QObjPtr::operator->()
{ return impl_->ptr(); }


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
    Threads::Locker lock( lock_ );
    set( sndr );
}

i_QPtrImpl::~i_QPtrImpl()
{
    Threads::Locker lock( lock_ );
    set( 0 );
}

