/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiedata.cc,v 1.19 2009-09-24 15:29:09 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "ioman.h"
#include "survinfo.h"
#include "wavelet.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"

#include "welltiedata.h"
#include "welltiesetup.h"
#include "welltied2tmodelmanager.h"
#include "welltiepickset.h"
#include "welltieunitfactors.h"

namespace WellTie
{

Log::Log( const char* nm )
    : Well::Log(nm)
    , arr_(0)
{
}


Log::~Log()
{
    delete arr_;
}


const Array1DImpl<float>* Log::getVal( const Interval<float>* si, bool dah )
{
    delete arr_; arr_ = 0;
    TypeSet<float> vals;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( si && dah_[idx]<si->start )
	    continue;
	float val = dah ? dah_[idx] : valArr()[idx];
	if ( si && dah_[idx]>=si->stop )
	    val =0;
	vals += val;
    }
    arr_ = new Array1DImpl<float> ( vals.size() );
    memcpy( arr_->getData(), vals.arr(), vals.size()*sizeof(float) );
    return arr_;
}


void Log::setVal( const Array1DImpl<float>* arr, bool isdah )
{
    if ( !arr ) return;

    TypeSet<float>& val = isdah ? dah_ : val_;
    val.erase();
    for ( int idx=0; idx<arr->info().getSize(0); idx++ )
	val.add( arr->get( idx ) );
}


LogSet::~LogSet()
{
    deepErase( logs );
}


const Array1DImpl<float>* LogSet::getVal( const char* nm, bool isdah, 
					  const Interval<float>* st ) const
{
    mDynCast(nm,return 0); return l->getVal(st,isdah);
}


void LogSet::resetData( const WellTie::Params::DataParams& params )
{
    deepErase( logs );
    for ( int idx=0; idx<params.colnms_.size(); idx++ )
	logs += new WellTie::Log( params.colnms_.get(idx) );
}


float LogSet::getExtremVal( const char* colnm, bool ismax ) const
{
    float maxval,             minval;
    maxval = get(colnm, 0);   minval = maxval;

    for ( int idz=0; idz<getLog(colnm)->size(); idz++)
    {
	float val =  get(colnm, idz);
	if ( maxval < val && !mIsUdf( val ) )
	    maxval = val;
	if ( minval > val && !mIsUdf(val) )
	    minval = val;
    }
    return ismax? maxval:minval;
}


DataHolder::DataHolder( WellTie::Params* params, Well::Data* wd, 
			const WellTie::Setup& s )
    	: params_(params)	
	, wd_(wd) 
	, setup_(s)
	, factors_(s.unitfactors_) 	   
{
    for ( int idx =0; idx<2; idx++ )
	wvltset_ += Wavelet::get( IOM().get(s.wvltid_) ); 
    uipms_   = &params_->uipms_;
    dpms_    = &params_->dpms_;
    pickmgr_ = new WellTie::PickSetMGR( wd_ );
    geocalc_ = new WellTie::GeoCalculator( *this );
    d2tmgr_  = new WellTie::D2TModelMGR( *this );
    logsset_ = new WellTie::LogSet( *this );
}


DataHolder::~DataHolder()
{
    delete logsset_;
    delete pickmgr_;
    delete d2tmgr_;
    delete params_;
}

}; //namespace WellTie
