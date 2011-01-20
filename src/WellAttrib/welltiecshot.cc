/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiecshot.cc,v 1.15 2011-01-20 10:21:38 cvsbruno Exp $";

#include "welltiecshot.h"

#include "idxable.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "welltiegeocalculator.h"

namespace WellTie
{

CheckShotCorr::CheckShotCorr( Well::Log& l, const Well::D2TModel& c, bool isvel)
    : log_(l)
    , cslog_(*new Well::Log)
{
    for ( int idx=0; idx<c.size(); idx++ )
       cslog_.addValue( c.dah(idx), c.value( idx ) );

    GeoCalculator geocalc;
    geocalc.velLogConv( cslog_, GeoCalculator::TWT2Vel );
    if ( !isvel )
	geocalc.velLogConv( cslog_, GeoCalculator::Vel2Son );
    calibrateLog2CheckShot( cslog_ );
}


CheckShotCorr::~CheckShotCorr()
{
    delete &cslog_;
}


void CheckShotCorr::calibrateLog2CheckShot( const Well::Log& cs ) 
{
    TypeSet<float> ctrlvals, calibratedpts, logvals, logdahs;      
    TypeSet<int> ctrlsamples;          
    for ( int idx=0; idx<cs.size(); idx++ )
    {
	int dahidx = log_.indexOf( cs.dah(idx) );
	if ( dahidx >= 0 )
	    { ctrlvals += cs.value( idx ); ctrlsamples += dahidx; }
    }

    const int logsz = log_.size();
    calibratedpts.setSize( logsz );
    IdxAble::callibrateArray( logvals.arr(), logsz,
	                      ctrlvals.arr(), ctrlsamples.arr(), 
			      ctrlvals.size(), false, calibratedpts.arr() );
    log_.erase(); 
    for ( int idx=0; idx<logsz; idx++ )
	log_.addValue( logdahs[idx], calibratedpts[idx] );
}

}; //namespace WellTie
