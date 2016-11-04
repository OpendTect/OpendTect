/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/

#include "welltied2tmodelmanager.h"

#include "welldata.h"
#include "welltiecshot.h"
#include "welltiedata.h"
#include "welltiegeocalculator.h"
#include "welltiesetup.h"

#include <math.h>

namespace WellTie
{

D2TModelMgr::D2TModelMgr( Well::Data& wd, DataWriter& dwr, const Setup& wts )
	: orgd2t_(0)
	, prvd2t_(0)
	, datawriter_(dwr)
	, wd_(&wd)
	, emptyoninit_(false)
{
    if ( mIsInvalidD2TM( wd ) )
	{ emptyoninit_ = true; wd.d2TModel().setEmpty(); }

    WellTie::GeoCalculator gc;
    Well::D2TModel* d2t = wts.useexistingd2tm_
			? &wd.d2TModel()
			: gc.getModelFromVelLog( wd, wts.vellognm_ );
    if ( !d2t )
    {
	errmsg_ = tr("Cannot generate depth/time model. Check your "
		     "velocity log");
	return;
    }

    if ( wts.corrtype_ == Setup::Automatic && wd_->haveCheckShotModel() )
	CheckShotCorr::calibrate( wd.checkShotModel(), *d2t );

    if ( !wts.useexistingd2tm_ )
	setAsCurrent( *d2t );
    if ( d2t != &wd.d2TModel() )
	delete d2t;

    orgd2t_ = emptyoninit_ ? 0 : new Well::D2TModel( wd.d2TModel() );
}


D2TModelMgr::~D2TModelMgr()
{
    delete prvd2t_;
    delete orgd2t_;
}


Well::D2TModel* D2TModelMgr::d2T()
{
    return wd_ ? &wd_->d2TModel() : 0;
}


void D2TModelMgr::shiftModel( float shift )
{
    if ( !d2T() )
	return;

    Well::D2TModel* d2t = new Well::D2TModel( *d2T() );
    d2t->shiftDahFrom( d2t->pointIDFor(1), shift );
    setAsCurrent( *d2t );
    delete d2t;
}


void D2TModelMgr::setAsCurrent( Well::D2TModel& d2t )
{
    if ( !wd_ )
	return;

    uiString msg;
    if ( !d2t.ensureValid(*wd_,msg) )
	return;

    if ( d2t.size() < 2 )
	return;

    delete prvd2t_; prvd2t_ = 0;
    if ( d2T() )
	prvd2t_ =  new Well::D2TModel( *d2T() );

    wd_->d2TModel() = d2t;
}


bool D2TModelMgr::undo()
{
    if ( !prvd2t_ )
	return false;
    setAsCurrent( *prvd2t_ );
    return true;
}


bool D2TModelMgr::cancel()
{
    if ( !wd_ )
	return false;

    if ( orgd2t_ )
	setAsCurrent( *orgd2t_ );
    return true;
}


bool D2TModelMgr::updateFromWD()
{
    if ( !wd_ || mIsInvalidD2TM( (*wd_) ) || !d2T() )
       return false;
    setAsCurrent( *d2T() );
    return true;
}


bool D2TModelMgr::commitToWD()
{
    if ( !datawriter_.writeD2TM() )
	return false;

    delete orgd2t_; orgd2t_ = 0;
    return true;
}


void D2TModelMgr::setFromData( const TypeSet<float>& dahs,
				const TypeSet<float>& times )
{
    Well::D2TModel* d2t = new Well::D2TModel();
    d2t->setData( dahs, times );
    setAsCurrent( *d2t );
    delete d2t;
}

} // namespace WellTie
