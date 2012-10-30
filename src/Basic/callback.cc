/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "callback.h"


CallBacker::CallBacker()
{}


CallBacker::CallBacker( const CallBacker& )
{}


CallBacker::~CallBacker()
{
    Threads::SpinLockLocker lock( cblock_ );
    
    for ( int idx=0; idx<listeners_.size(); idx++ )
	listeners_[idx]->removeListener( this );
    
    for ( int idx=0; idx<attachednotifiers_.size(); idx++ )
	attachednotifiers_[idx]->removeWith( this );
}


void CallBacker::attachCB(NotifierAccess& notif, const CallBack& cb )
{
    notif.notify( cb );
 
    if ( cb.cbObj()!=this )
	return;

    notif.cber_->addListener( this );
    
    Threads::SpinLockLocker lock( cblock_ );
    if ( attachednotifiers_.indexOf( &notif )==-1 )
	attachednotifiers_ += &notif;
 
    listeners_ += notif.cber_;
}


void CallBacker::addListener( CallBacker* cb )
{
    Threads::SpinLockLocker lock( cblock_ );
    listeners_ += cb;
}


void CallBacker::removeListener( CallBacker* cb )
{
    Threads::SpinLockLocker lock( cblock_ ); 
    listeners_ -= cb;
    
    for ( int idx=attachednotifiers_.size()-1; idx>=0; idx-- )
    {
	if ( attachednotifiers_[idx]->cber_==cb )
	    attachednotifiers_.removeSingle( idx );
    }
}


void CallBack::doCall( CallBacker* cber )
{
    if ( obj_ && fn_ )
	(obj_->*fn_)( cber );
    else if ( sfn_ )
	sfn_( cber );
}

void CallBackSet::doCall( CallBacker* obj, const bool* enabledflag,
			  CallBacker* exclude )
{
    const bool enab = true;
    const bool& enabled = enabledflag ? *enabledflag : enab;
    if ( !enabled ) return;

    TypeSet<CallBack> cbscopy = *this;
    for ( int idx=0; idx<cbscopy.size(); idx++ )
    {
	CallBack& cb = cbscopy[idx];
	if ( indexOf(cb)==-1 )
	    continue;

	if ( !exclude || cb.cbObj()!=exclude )
	    cb.doCall( obj );
    }
}


void CallBackSet::removeWith( CallBacker* cbrm )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.cbObj() == cbrm )
	    { removeSingle( idx ); idx--; }
    }
}


void CallBackSet::removeWith( CallBackFunction cbfn )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.cbFn() == cbfn )
	    { removeSingle( idx ); idx--; }
    }
}


void CallBackSet::removeWith( StaticCallBackFunction cbfn )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.scbFn() == cbfn )
	    { removeSingle( idx ); idx--; }
    }
}


NotifierAccess::NotifierAccess()
    : enabled_(true)		
{}


void NotifierAccess::notify( const CallBack& cb, bool first )	
{ 
    if ( first ) 
	cbs_.insert(0,cb); 
    else
	cbs_ += cb; 
}


void NotifierAccess::notifyIfNotNotified( const CallBack& cb )
{ if ( cbs_.indexOf(cb)==-1 ) notify(cb); }


void NotifierAccess::remove( const CallBack& cb )
{ cbs_ -= cb; }


void NotifierAccess::removeWith( CallBacker* cb )
{
    if ( cber_ == cb )
	{ cbs_.erase(); cber_ = 0; return; }

    cbs_.removeWith( cb );
}
