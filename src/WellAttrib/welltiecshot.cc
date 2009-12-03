/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiecshot.cc,v 1.14 2009-12-03 16:25:39 cvsbruno Exp $";

#include "welltiecshot.h"

#include "idxable.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "welltiedata.h"
#include "welld2tmodel.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"
#include "welltiegeocalculator.h"

namespace WellTie
{

CheckShotCorr::CheckShotCorr( WellTie::DataHolder& dh )
	: log_(new Well::Log(*dh.wd()->logs().getLog(dh.setup().vellognm_)))
	, cs_(dh.wd()->checkShotModel())
{
    if ( !cs_ || !cs_->size() )
	return;

    for ( int idx=1; idx<cs_->size(); idx++ )
    {
	if ( cs_->dah(idx) == cs_->dah(idx-1) )
	    cs_->remove( idx );
    }
    
    TypeSet<float> newcsvals; 

    WellTie::GeoCalculator geocalc( dh );
    geocalc.checkShot2Log( cs_, dh.setup().issonic_, newcsvals );
    calibrateLog2CheckShot( newcsvals, geocalc );

    log_->setName( dh.setup().corrvellognm_ );
    dh.wd()->logs().add( log_ );
}


void CheckShotCorr::calibrateLog2CheckShot( const TypeSet<float>& csvals, 
				       WellTie::GeoCalculator& geocalc )
{
    const int logsz = log_->size();
    TypeSet<float> ctrlvals, calibratedpts, logvals, logdahs;      
    calibratedpts.setSize( logsz );
    TypeSet<int> ctrlsamples;          
    
    for ( int idx=0; idx<logsz; idx++ )
    {
	logvals += log_->value(idx);
	logdahs += log_->dah(idx);
    }

    geocalc.interpolateLogData( logdahs, log_->dahStep(true), true );
    geocalc.interpolateLogData( logvals, log_->dahStep(true), false );

    for ( int idx=0; idx<csvals.size(); idx++ )
    {
	int dahidx = log_->indexOf( cs_->dah(idx) );
	if ( dahidx >0 )
	{
	    ctrlvals += csvals[idx];
	    ctrlsamples += dahidx;
	}
    }

    IdxAble::callibrateArray( logvals.arr(), logsz,
	                      ctrlvals.arr(), ctrlsamples.arr(), 
			      ctrlvals.size(), false, calibratedpts.arr() );
    
    for ( int idx=0; idx<logsz; idx++ )
    {
	float calibratedval = calibratedpts[idx];
	if ( mIsUdf( calibratedval ) )
	    calibratedval = idx ? calibratedpts[idx-1] : log_->valArr()[0]; 
	log_->valArr()[idx] = calibratedval; 
	log_->dahArr()[idx] = logdahs[idx];
    }
}

}; //namespace WellTie
