/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltied2tmodelmanager.cc,v 1.26 2010-12-07 12:47:19 cvsbruno Exp $";

#include "welltied2tmodelmanager.h"

#include "file.h"
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

namespace WellTie
{

D2TModelMGR::D2TModelMGR( WellTie::DataHolder& dh )
	: dholder_(dh)
	, geocalc_(*dh.geoCalc())
	, params_(*dh.params())
	, orgd2t_(0)					    
	, prvd2t_(0)
	, emptyoninit_(false)
	, datawriter_(new WellTie::DataWriter(dh))
{
    if ( !wd() ) return;

    if ( mIsUnvalidD2TM((*wd())) )
    {
	emptyoninit_ = true;
	wd()->setD2TModel( new Well::D2TModel );
    }
    orgd2t_ = emptyoninit_ ? 0 : new Well::D2TModel( *wd()->d2TModel() );
    
    //apply check shot correction on sonic log
    if ( wd()->haveCheckShotModel() && !dh.setup().useexistingd2tm_ )
	WellTie::CheckShotCorr cscorr( dh );

    if ( (emptyoninit_ || wd()->haveCheckShotModel()) 
	    			&& !dh.setup().useexistingd2tm_ )
	setFromVelLog( dh.params()->dpms_.currvellognm_, true );

    ensureValid( d2T() );
} 


D2TModelMGR::~D2TModelMGR()
{
    delete datawriter_;
    if ( prvd2t_ ) delete prvd2t_;
}


Well::Data* D2TModelMGR::wd()
{
    return dholder_.wd();
}


Well::D2TModel& D2TModelMGR::d2T()
{
    return *wd()->d2TModel();
}

#define mRemoveSameTimeValues(d2tm)\
    for ( int idx=1; idx<d2tm->size(); idx++ )\
    {\
	if ( mIsZero(d2tm->value(idx)-d2tm->value(idx-1),1e-8) )\
	    d2tm->remove( idx-1 );\
    }
void D2TModelMGR::setFromVelLog( const char* lognm,  bool docln )
{
    Well::D2TModel* d2tm = geocalc_.getModelFromVelLog(lognm,docln);
    if ( !d2tm ) return;

    setAsCurrent( d2tm );
    applyCheckShotShiftToModel();
}


void D2TModelMGR::applyCheckShotShiftToModel()
{
    if ( !params_.uipms_.iscscorr_ ) return;
    
    Well::D2TModel* d2tm = &d2T();

    const Well::D2TModel* cs = wd()->checkShotModel();
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


void D2TModelMGR::shiftModel( float shift)
{
    Well::D2TModel* d2t = new Well::D2TModel( d2T() );

    for ( int dahidx=1; dahidx<d2t->size(); dahidx++ )
	d2t->valArr()[dahidx] += shift;
    
    mRemoveSameTimeValues(d2t);
    setAsCurrent( d2t );
}


void D2TModelMGR::replaceTime( const Array1DImpl<float>& timevals )
{
    Well::D2TModel* d2t = new Well::D2TModel( d2T() );
    for ( int dahidx=1; dahidx<d2t->size(); dahidx++ )
	d2t->valArr()[dahidx] = timevals[dahidx];

    mRemoveSameTimeValues(d2t);
    setAsCurrent( d2t );
}


void D2TModelMGR::setAsCurrent( Well::D2TModel* d2t )
{
    ensureValid( *d2t );
    if ( !d2t || d2t->size() < 1 || d2t->value(1)<0 )
    { pErrMsg("Bad D2TMdl: ignoring"); delete d2t; return; }

    if ( prvd2t_ )
	delete prvd2t_; 
    prvd2t_ =  new Well::D2TModel( d2T() );
    wd()->setD2TModel( d2t );
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
	wd()->d2TModel()->erase();	
    else
	setAsCurrent( orgd2t_ );
    wd()->d2tchanged.trigger();
    return true;
}


bool D2TModelMGR::updateFromWD()
{
    if ( mIsUnvalidD2TM( (*wd())) )
       return false;	
    setAsCurrent( wd()->d2TModel() );
    return true;
}


bool D2TModelMGR::commitToWD()
{
    if ( !datawriter_->writeD2TM() ) 
	return false;

    wd()->d2tchanged.trigger();
    if ( orgd2t_ && !emptyoninit_ )
	delete orgd2t_;

    return true;
}


void D2TModelMGR::ensureValid( Well::D2TModel& d2t )
{
    geocalc_.ensureValidD2TModel( d2t );
}


}; //namespace WellTie
