/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiecshot.cc,v 1.18 2011-07-15 12:01:12 cvsbruno Exp $";

#include "welltiecshot.h"

#include "idxable.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "welltiegeocalculator.h"

namespace WellTie
{

CheckShotCorr::CheckShotCorr( Well::Log& l, const Well::D2TModel& c, bool isson)
    : log_(l)
    , cslog_(*new Well::Log)
{
    GeoCalculator geocalc;
    geocalc.d2TModel2Log( c, cslog_ );
    geocalc.velLogConv( cslog_, GeoCalculator::TWT2Vel );
    cslog_.setUnitMeasLabel( log_.unitMeasLabel() );
    if ( isson )
	geocalc.velLogConv( cslog_, GeoCalculator::Vel2Son );
    calibrateLog2CheckShot( cslog_ );
}


CheckShotCorr::~CheckShotCorr()
{
    delete &cslog_;
}


void CheckShotCorr::calibrateLog2CheckShot( const Well::Log& cs ) 
{
    TypeSet<float> ctrlvals, calibratedvals, logdahs, logvals;    
    TypeSet<int> ctrlsamples;

    int logsz = log_.size();
    int csidx = 0;
    const float startdpt = log_.dah( 0 );
    while ( startdpt > cs.dah(csidx) )
    {
	logvals += cs.value(csidx);
	logdahs += cs.dah(csidx); 
	csidx ++;
    }
    for ( int idx=0; idx<logsz; idx++ )
    {
	logvals += log_.value(idx);
	logdahs += log_.dah( idx );
    }
    csidx = cs.size() -1;
    const float stopdpt = log_.dah(log_.size()-1 );
    logsz = logvals.size();
    while ( csidx && cs.dah(csidx) > stopdpt ) 
    {
	logvals.insert( logsz-1, cs.value(csidx ) );
	logdahs.insert( logsz-1, cs.dah(csidx) );
	csidx --;
    }
    logsz = logvals.size();

    for ( int idx=0; idx<cs.size(); idx++ )
    {
	int dahidx = -1;
	IdxAble::findFPPos( logdahs.arr(), logsz, cs.dah(idx), -1, dahidx );
	if ( dahidx >= 0 )
	    { ctrlvals += cs.value( idx ); ctrlsamples += dahidx; }
    }

    calibratedvals.setSize( logsz );
    IdxAble::callibrateArray( logvals.arr(), logsz,
	                      ctrlvals.arr(), ctrlsamples.arr(), 
			      ctrlvals.size(), false, calibratedvals.arr() );
    log_.erase(); 
    for ( int idx=0; idx<logsz; idx++ )
	log_.addValue( logdahs[idx], calibratedvals[idx] );
}

}; //namespace WellTie
