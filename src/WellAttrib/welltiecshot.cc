/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiecshot.cc,v 1.16 2011-02-04 14:00:54 cvsbruno Exp $";

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
    GeoCalculator geocalc;
    geocalc.d2TModel2Log( c, cslog_ );
    geocalc.velLogConv( cslog_, GeoCalculator::TWT2Vel );
    if ( !isvel )
	geocalc.velLogConv( log_, GeoCalculator::Son2Vel );
    calibrateLog2CheckShot( cslog_ );
}


CheckShotCorr::~CheckShotCorr()
{
    delete &cslog_;
}


void CheckShotCorr::calibrateLog2CheckShot( const Well::Log& cs ) 
{
    TypeSet<float> ctrlvals, calibratedvals, logdahs;      
    TypeSet<int> ctrlsamples;          
    for ( int idx=0; idx<cs.size(); idx++ )
    {
	int dahidx = log_.indexOf( cs.dah(idx) );
	if ( dahidx >= 0 )
	    { ctrlvals += cs.value( idx ); ctrlsamples += dahidx; }
    }
    const int logsz = log_.size();
    for ( int idx=0; idx<logsz; idx++ )
	logdahs += log_.dah( idx );

    calibratedvals.setSize( logsz );
    IdxAble::callibrateArray( log_.valArr(), logsz,
	                      ctrlvals.arr(), ctrlsamples.arr(), 
			      ctrlvals.size(), false, calibratedvals.arr() );
    log_.erase(); 
    for ( int idx=0; idx<logsz; idx++ )
	log_.addValue( logdahs[idx], calibratedvals[idx] );
}

}; //namespace WellTie
