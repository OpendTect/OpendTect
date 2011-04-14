/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltied2tmodelmanager.cc,v 1.29 2011-04-14 13:51:06 cvsbruno Exp $";

#include "welltied2tmodelmanager.h"

#include "arrayndimpl.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "welltiecshot.h"
#include "welltiedata.h"
#include "welltiesetup.h"

#include <math.h>

namespace WellTie
{

D2TModelMgr::D2TModelMgr( Well::Data& wd, const Setup& su )
	: orgd2t_(0)					    
	, prvd2t_(0)
	, issonic_(su.issonic_)
	, wd_(&wd)			 
{
    if ( mIsUnvalidD2TM( wd ) )
	{ emptyoninit_ = true; wd.setD2TModel( new Well::D2TModel ); }

    if ( emptyoninit_ || !su.useexisting_ )
	setFromVelLog( su.currvellog_ );

    orgd2t_ = emptyoninit_ ? 0 : new Well::D2TModel( *wd.d2TModel() );
    ensureValid( *d2T() );
} 


D2TModelMgr::~D2TModelMgr()
{
    if ( prvd2t_ ) delete prvd2t_;
}


Well::D2TModel* D2TModelMgr::d2T()
{
    return wd_ ? wd_->d2TModel() : 0;
}


void D2TModelMgr::setFromVelLog( const char* lognm )
{
    Well::Log* log = wd_ ? wd_->logs().getLog( lognm ) : 0;
    if ( !log ) return;

    const Well::Info& info = wd_->info();
    float surf = -info.surfaceelev;
    if ( mIsUdf( fabs(surf) ) ) surf = 0;

    Well::D2TModel* d2tm = 
	calc_.getModelFromVelLog( *log, &wd_->track(), surf, issonic_ );

    if ( !d2tm ) return;

    setAsCurrent( d2tm );
    applyCheckShotShiftToModel();
}


void D2TModelMgr::applyCheckShotShiftToModel()
{
    Well::D2TModel* d2tm = d2T();
    if ( !d2tm ) return;

    const Well::D2TModel* cs = wd_ ? wd_->checkShotModel() : 0; 
    if ( !cs ) return;

    float csshift = 0;
    for ( int idx=0; idx<cs->size(); idx++ )
    {
	if ( cs->dah(idx) >= d2tm->dah(1) )
	{
	    csshift = cs->value(idx) - d2tm->getTime( cs->dah(idx) ); 
	    break;
	}
    }
    shiftModel( csshift );
}


#define mRemoveSameTimeValues(d2tm)\
    for ( int idx=1; idx<d2tm->size(); idx++ )\
    {\
	if ( mIsZero(d2tm->value(idx)-d2tm->value(idx-1),1e-8) )\
	    d2tm->remove( idx-1 );\
    }
void D2TModelMgr::shiftModel( float shift)
{
    if ( !d2T() ) return;
    Well::D2TModel* d2t = new Well::D2TModel( *d2T() );

    for ( int dahidx=1; dahidx<d2t->size(); dahidx++ )
	d2t->valArr()[dahidx] += shift;
    
    mRemoveSameTimeValues(d2t);
    setAsCurrent( d2t );
}


void D2TModelMgr::replaceTime( const Array1DImpl<float>& timevals )
{
    if ( !d2T() ) return;
    Well::D2TModel* d2t = new Well::D2TModel( *d2T() );
    for ( int dahidx=1; dahidx<d2t->size(); dahidx++ )
	d2t->valArr()[dahidx] = timevals[dahidx];

    mRemoveSameTimeValues(d2t);
    setAsCurrent( d2t );
}


void D2TModelMgr::setAsCurrent( Well::D2TModel* d2t )
{
    ensureValid( *d2t );
    if ( !d2t || d2t->size() < 2 || d2t->value(1)<0 )
    { pErrMsg("Bad D2TMdl: ignoring"); delete d2t; return; }

    if ( prvd2t_ )
	delete prvd2t_; 
    if ( !d2T() ) 
	prvd2t_ =  new Well::D2TModel( *d2T() );
    if ( wd_ )
    {
	wd_->setD2TModel( d2t );
	wd_->d2tchanged.trigger();
    }
}


bool D2TModelMgr::undo()
{
    if ( !prvd2t_ ) return false; 
    Well::D2TModel* tmpd2t =  new Well::D2TModel( *prvd2t_ );
    setAsCurrent( tmpd2t );
    return true;
}


bool D2TModelMgr::cancel()
{
    if ( !wd_ ) return false;
    if ( emptyoninit_ && d2T() )
	d2T() ->erase();	
    else
	setAsCurrent( orgd2t_ );
    wd_->d2tchanged.trigger();
    return true;
}


bool D2TModelMgr::updateFromWD()
{
    if ( !wd_ || mIsUnvalidD2TM( (*wd_) ) || !d2T() )
       return false;	
    setAsCurrent( d2T() );
    return true;
}


bool D2TModelMgr::commitToWD()
{
    //TODO 
    /*
    if ( !datawriter_.writeD2TM() ) 
	return false;
    */

    if ( wd_ ) wd_->d2tchanged.trigger();
    if ( orgd2t_ && !emptyoninit_ )
	delete orgd2t_;

    return true;
}


void D2TModelMgr::ensureValid( Well::D2TModel& d2t )
{
    calc_.ensureValidD2TModel( d2t );
}


}; //namespace WellTie
