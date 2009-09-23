/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiedata.cc,v 1.17 2009-09-23 11:50:08 cvsbruno Exp $";

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


const Array1DImpl<float>* Log::getVal() 
{
    delete arr_; arr_ = 0;
    if ( size() ) arr_ = new Array1DImpl<float>( size() );
    for ( int idx=0; idx<size(); idx++ )
	arr_->set( idx, valArr()[idx] );
    return arr_;
}


const Array1DImpl<float>* Log::getDah()
{
    delete arr_; arr_ = 0;
    if ( size() ) arr_ = new Array1DImpl<float>( size() );
    for ( int idx=0; idx<size(); idx++ )
	arr_->set( idx, dah(idx) );
    return arr_;
}


void Log::setVal( const Array1DImpl<float>* arr )
{
    if ( !arr ) return;

    val_.erase();
    for ( int idx=0; idx<size(); idx++ )
	val_.add( arr->get( idx ) );
}


void Log::setDah( const Array1DImpl<float>* arr )
{
    if ( !arr ) return;

    dah_.erase(); 
    for ( int idx=0; idx<arr->info().getSize(0); idx++ )
	dah_.add( arr->get( idx ) );
}


void Log::resample( int step )
{
    const int orgsize = size();

    const float* orgvals = valArr();	const float* orgdah = dah_.arr();
    val_.erase();			dah_.erase();

    for ( int idx=0; idx<int(orgsize/step); idx ++ )
	addValue( orgdah[idx*step], orgvals[idx*step] );
}


LogSet::~LogSet()
{
    deepErase( logs );
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
