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
    return impl_->lock_;
}


NotifierAccess& QObjPtr::deleteNotifier()
{
    return impl_->notifier_;
}