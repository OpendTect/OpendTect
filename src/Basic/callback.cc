/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2011
-*/

static const char* rcsID = "$Id: callback.cc,v 1.1 2011/09/16 09:47:35 cvsbert Exp $";

#include "callback.h"


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
	    { remove( idx ); idx--; }
    }
}


void CallBackSet::removeWith( CallBackFunction cbfn )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.cbFn() == cbfn )
	    { remove( idx ); idx--; }
    }
}


void CallBackSet::removeWith( StaticCallBackFunction cbfn )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	CallBack& cb = (*this)[idx];
	if ( cb.scbFn() == cbfn )
	    { remove( idx ); idx--; }
    }
}


void i_Notifier::removeWith( CallBacker* cb )
{
    if ( cber_ == cb )
	{ cbs_.erase(); cber_ = 0; return; }

    cbs_.removeWith( cb );
}
