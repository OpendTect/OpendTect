/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2014
________________________________________________________________________

-*/

#include "uivismod.h"

#include "moddepmgr.h"
#include "uimsg.h"
#include "visusershowwaitimpl.h"

class uiVisUserShowWaitPoster : public visBase::UserShowWaitPoster
{
public:

uiVisUserShowWaitPoster( const visBase::Scene* s, int fld )
    : sbfld_(fld)
    , usw_(0)
    , uiparent_(0) // if scenes can be outside OpendTect main window, look up
{
}

~uiVisUserShowWaitPoster()
{
    delete usw_;
}

void post( const uiString& msg )
{
    if ( !usw_ )
	usw_ = new uiUserShowWait( uiparent_, msg, sbfld_ );
    else
	usw_->setMessage( msg );
}

    uiParent*		uiparent_;
    uiUserShowWait*	usw_;
    const int		sbfld_;

};

class uiVisUserShowWaitPosterFactory : public visBase::UserShowWaitPosterFactory
{
public:

visBase::UserShowWaitPoster* getPoster( const visBase::Scene* s, int fld ) const
{ return new uiVisUserShowWaitPoster( s, fld ); }

};


mDefModInitFn(uiVis)
{
    mIfNotFirstTime( return );

    visBase::UserShowWaitImpl::setPosterFactory(
				new uiVisUserShowWaitPosterFactory );
}
