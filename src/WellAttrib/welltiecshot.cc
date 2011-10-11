/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiecshot.cc,v 1.20 2011-10-11 14:51:34 cvsbruno Exp $";

#include "welltiecshot.h"

#include "idxable.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "welltiegeocalculator.h"

namespace WellTie
{

CheckShotCorr::CheckShotCorr( Well::Log& log, float startdah, 
				const Well::D2TModel& c, bool isson)
{
    Well::Log cslog;
    GeoCalculator geocalc;
    geocalc.d2TModel2Log( c, cslog );
    if ( isson )
    {
	geocalc.son2TWT( log, true, startdah );
	calibrateLog2Log( cslog, log );
	geocalc.son2TWT( log, false, startdah );
    }
    else
    {
	geocalc.vel2TWT( log, true, startdah );
	calibrateLog2Log( cslog, log );
	geocalc.vel2TWT( log, false, startdah );
    }
}


void CheckShotCorr::calibrateLog2Log( const Well::Log& cs, Well::Log& log ) 
{
    TypeSet<float> ctrlvals, calibratedvals, logdahs, logvals;    
    TypeSet<int> ctrlsamples;

    int logsz = log.size();
    int csidx = 0;
    const float startdpt = log.dah( 0 );
    while ( startdpt > cs.dah(csidx) )
    {
	logvals += cs.value(csidx);
	logdahs += cs.dah(csidx); 
	csidx ++;
    }
    for ( int idx=0; idx<logsz; idx++ )
    {
	logvals += log.value(idx);
	logdahs += log.dah( idx );
    }
    csidx = cs.size() -1;
    const float stopdpt = log.dah(log.size()-1 );
    logsz = logvals.size();
    while ( csidx && cs.dah(csidx) > stopdpt ) 
    {
	logvals.insert( logsz-1, cs.value(csidx ) );
	logdahs.insert( logsz-1, cs.dah(csidx) );
	csidx --;
    }
    logsz = logvals.size();

    sort_array( logvals.arr(), logsz ); 
    sort_array( logdahs.arr(), logsz );
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
    log.erase(); 
    for ( int idx=0; idx<logsz; idx++ )
	log.addValue( logdahs[idx], calibratedvals[idx] );
}

}; //namespace WellTie
