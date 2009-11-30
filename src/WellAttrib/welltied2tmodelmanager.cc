/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltied2tmodelmanager.cc,v 1.16 2009-11-30 16:33:14 cvsbruno Exp $";

#include "welltied2tmodelmanager.h"

#include "filegen.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welllog.h"
#include "wellman.h"
#include "welllogset.h"
#include "welltrack.h"

#include "welltiegeocalculator.h"
#include "welltiecshot.h"
#include "welltiedata.h"
#include "welltiesetup.h"

#define mMinNrTimeSamples 5
namespace WellTie
{

D2TModelMGR::D2TModelMGR( WellTie::DataHolder& dh )
	: wd_(dh.wd())
	, geocalc_(*dh.geoCalc())
	, orgd2t_(0)					    
	, prvd2t_(0)
	, emptyoninit_(false)
	, wtsetup_(dh.setup())	
	, datawriter_(new WellTie::DataWriter(&dh))
	, iscscorr_(dh.uipms()->iscscorr_) 
{
    if ( !wd_ ) return;
    if ( !wd_->d2TModel() || wd_->d2TModel()->size() <= mMinNrTimeSamples )
    {
	emptyoninit_ = true;
	wd_->setD2TModel( new Well::D2TModel );
    }
    orgd2t_ = emptyoninit_ ? 0 : new Well::D2TModel( *wd_->d2TModel() );

    if ( wd_->haveCheckShotModel() )
	WellTie::CheckShotCorr cscorr( dh );
    setFromVelLog( dh.params()->dpms_.currvellognm_, true );
} 


D2TModelMGR::~D2TModelMGR()
{
    delete datawriter_;
    if ( prvd2t_ ) delete prvd2t_;
}


Well::D2TModel& D2TModelMGR::d2T()
{
    return *wd_->d2TModel();
}


void D2TModelMGR::setFromVelLog( const char* lognm,  bool docln )
{
    Well::D2TModel* d2tm = geocalc_.getModelFromVelLog(lognm,docln);
    if ( !d2tm ) return;

    const float startdah = iscscorr_? wd_->checkShotModel()->dah(0)
				    : wd_->track().dah(0)-wd_->track().value(0);
    for ( int idx=d2tm->size()-1; idx>=1; idx-- )
    {
	if ( startdah > d2tm->dah( idx ) )
	    d2tm->remove( idx );
    }
    setAsCurrent( d2tm );

    if ( iscscorr_ ) 
	shiftModel( wd_->checkShotModel()->value(0) - d2tm->value(1) );
}


void D2TModelMGR::shiftModel( float shift)
{
    Well::D2TModel* d2t = new Well::D2TModel( d2T() );

    for ( int dahidx=1; dahidx<d2t->size(); dahidx++ )
	d2t->valArr()[dahidx] += shift;
    
    setAsCurrent( d2t );
}


void D2TModelMGR::replaceTime( const Array1DImpl<float>& timevals )
{
    Well::D2TModel* d2t = new Well::D2TModel( d2T() );
    for ( int dahidx=1; dahidx<d2t->size(); dahidx++ )
	d2t->valArr()[dahidx] = timevals[dahidx];

    setAsCurrent( d2t );
}


void D2TModelMGR::setAsCurrent( Well::D2TModel* d2t )
{
    if ( !d2t || d2t->size() < 1 || d2t->value(1)<0 )
    { pErrMsg("Bad D2TMdl: ignoring"); delete d2t; return; }

    if ( prvd2t_ )
	delete prvd2t_; 
    prvd2t_ =  new Well::D2TModel( d2T() );
    wd_->setD2TModel( d2t );
}


bool D2TModelMGR::undo()
{
    if ( !prvd2t_ ) return false; 
    Well::D2TModel* tmpd2t =  new Well::D2TModel( *prvd2t_ );
    setAsCurrent( tmpd2t );
    return true;
}


bool D2TModelMGR::cancel()
{
    if ( emptyoninit_ )
	wd_->d2TModel()->erase();	
    else
	setAsCurrent( orgd2t_ );
    wd_->d2tchanged.trigger();
    return true;
}


bool D2TModelMGR::updateFromWD()
{
    if ( !wd_->d2TModel() || wd_->d2TModel()->size()<1 )
       return false;	
    setAsCurrent( wd_->d2TModel() );
    return true;
}


bool D2TModelMGR::commitToWD()
{
    if ( !datawriter_->writeD2TM() ) 
	return false;

    wd_->d2tchanged.trigger();
    if ( orgd2t_ && !emptyoninit_ )
	delete orgd2t_;

    return true;
}


}; //namespace WellTie
