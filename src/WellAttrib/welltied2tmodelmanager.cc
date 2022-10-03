/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welltied2tmodelmanager.h"


#include "idxable.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welltiedata.h"
#include "welltiegeocalculator.h"
#include "welltiesetup.h"

#include <math.h>

void WellTie::calibrate( const Well::D2TModel& cs, Well::D2TModel& d2t )
{
    const int d2tsz = d2t.size();
    if ( d2tsz < 2 ) return;

    float* d2tarr = d2t.valArr();
    float* daharr = d2t.dahArr();
    bool found = false;

    sort_array( d2tarr, d2tsz );
    sort_array( daharr, d2tsz );

    for ( int idx=0; idx<cs.size(); idx++ )
    {
	if ( mIsUdf(cs.dah(idx)) ) continue;
	found = false;
	for ( int insertidx=0; insertidx<d2tsz; insertidx++ )
	{
	    if ( mIsEqual(cs.dah(idx),d2t.dah(insertidx),1e-3) )
	    {
		found = true;
		break;
	    }

	    if ( d2t.dah(insertidx) > cs.dah(idx) )
		break;
	}

	if ( !found )
	    d2t.insertAtDah( cs.dah(idx), cs.value(idx) );
    }

    TypeSet<int> ctrlsamples;
    d2tarr = d2t.valArr();
    daharr = d2t.dahArr();
    for ( int idx=0; idx<cs.size(); idx++ )
    {
	const int dahidx = d2t.indexOf( cs.dah(idx) );
	ctrlsamples += dahidx;
    }
    const float* csarr = cs.valArr();
    IdxAble::calibrateArray( d2tarr, d2tsz,
			      csarr, ctrlsamples.arr(),
			      cs.size(), false, d2tarr );
}


// WellTie::D2TModelMgr

WellTie::D2TModelMgr::D2TModelMgr( Well::Data& wd, DataWriter& dwr,
				   const Setup& wts )
    : datawriter_(dwr)
    , wd_(&wd)
{
    if ( mIsUnvalidD2TM( wd_ ) )
	{ emptyoninit_ = true; wd_->setD2TModel( new Well::D2TModel ); }

    Well::D2TModel* d2t = wts.useexistingd2tm_
	    ? wd_->d2TModel()
	    : WellTie::GeoCalculator::getModelFromVelLog( *wd_, wts.vellognm_ );
    if ( !d2t )
    {
	errmsg_ = tr("Cannot generate depth/time model. Check your "
		     "velocity log");
	return;
    }

    if ( wts.corrtype_ == Setup::Automatic && wd_->haveCheckShotModel() )
	calibrate( *wd_->checkShotModel(), *d2t );

    if ( !wts.useexistingd2tm_ )
	setAsCurrent( d2t );

    orgd2t_ = emptyoninit_ ? nullptr : new Well::D2TModel( *wd_->d2TModel() );
}


WellTie::D2TModelMgr::~D2TModelMgr()
{
    delete prvd2t_;
}


Well::D2TModel* WellTie::D2TModelMgr::d2T()
{
    return wd_ ? wd_->d2TModel() : nullptr;
}


#define mRemoveSameTimeValues(d2tm)\
    for ( int idx=1; idx<d2tm->size(); idx++ )\
    {\
	if ( mIsZero(d2tm->value(idx)-d2tm->value(idx-1),1e-8) )\
	    d2tm->remove( idx-1 );\
    }

void WellTie::D2TModelMgr::shiftModel( float shift)
{
    if ( !d2T() ) return;
    Well::D2TModel* d2t = new Well::D2TModel( *d2T() );

    for ( int dahidx=1; dahidx<d2t->size(); dahidx++ )
	d2t->valArr()[dahidx] += shift;

    mRemoveSameTimeValues(d2t);
    setAsCurrent( d2t );
}


void WellTie::D2TModelMgr::setAsCurrent( Well::D2TModel* d2t )
{
    if ( !d2t || !wd_ )
	return;

    uiString msg;
    if ( !d2t->ensureValid(*wd_,msg) )
	{ delete d2t; return; }

    if ( d2t->size() < 2 )
	{ delete d2t; return; }

    deleteAndZeroPtr( prvd2t_ );
    if ( d2T() )
	prvd2t_ =  new Well::D2TModel( *d2T() );

    wd_->setD2TModel( d2t );
}


bool WellTie::D2TModelMgr::undo()
{
    if ( !prvd2t_ )
	return false;

    auto* tmpd2t =  new Well::D2TModel( *prvd2t_ );
    setAsCurrent( tmpd2t );
    return true;
}


bool WellTie::D2TModelMgr::cancel()
{
    if ( !wd_ ) return false;

    setAsCurrent( orgd2t_ );
    wd_->d2tchanged.trigger();

    return true;
}


bool WellTie::D2TModelMgr::updateFromWD()
{
    if ( !wd_ || mIsUnvalidD2TM( (wd_) ) || !d2T() )
       return false;

    setAsCurrent( d2T() );
    return true;
}


bool WellTie::D2TModelMgr::commitToWD()
{
    if ( !datawriter_.writeD2TM() )
	return false;

    if ( wd_ ) wd_->d2tchanged.trigger();
    deleteAndZeroPtr( orgd2t_ );

    return true;
}


void WellTie::D2TModelMgr::setFromData( float* dahs, float* times, int sz )
{
    if ( !d2T() ) return;
    auto* d2t = new Well::D2TModel();
    d2t->add( d2T()->dah(0), d2T()->value(0) );
    for ( int dahidx=1; dahidx<sz; dahidx++ )
	d2t->add( dahs[dahidx], times[dahidx] );

    mRemoveSameTimeValues(d2t);
    setAsCurrent( d2t );
}
