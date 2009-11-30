/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiecshot.cc,v 1.12 2009-11-30 16:33:14 cvsbruno Exp $";

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
    setCSToLogScale( newcsvals, dh.getUnits().velFactor(), geocalc );
    calibrateLogToCS( newcsvals, geocalc );
    log_->setName( dh.setup().corrvellognm_ );
    dh.wd()->logs().add( log_ );
}


void CheckShotCorr::setCSToLogScale( TypeSet<float>& cstolog, double velfactor,
				      WellTie::GeoCalculator& geocalc )
{
    TypeSet<float> dpt, csvals;
    for ( int idx=0; idx<cs_->size(); idx++ )
    {
	dpt += cs_->dah(idx);
	csvals += cs_->value(idx);
    }
    geocalc.TWT2Vel( csvals, dpt, cstolog, true );
}


void CheckShotCorr::calibrateLogToCS( const TypeSet<float>& csvals, 
				       WellTie::GeoCalculator& geocalc )
{
    TypeSet<float> logvaldah, coeffs, logshifts, csshifts;

    //add extra log values if CheckShot start dah < log start dah
    const float startdah = log_->dah(0); 
    const float stopdah = log_->dah(log_->size()-1);
    for ( int idx=0; idx<cs_->size(); idx++)
    {
	const float csdah = cs_->dah(idx);
	if ( csdah < startdah )
	    log_->insertAtDah( csdah, csvals[idx] );
    }

    for ( int idx=0; idx<cs_->size(); idx++)
    {
	float val = log_->getValue(cs_->dah(idx));
	if ( mIsUdf(val) )
	    logvaldah += csvals[idx];
	else
	    logvaldah += val; 
    }

    geocalc.interpolateLogData ( logvaldah, log_->dahStep(true),  false );

    for ( int idx=0; idx<cs_->size(); idx++)
	csshifts += csvals[idx]-logvaldah[idx];

    for ( int idx=0; idx<cs_->size()-1; idx++)
	coeffs += (csshifts[idx+1]-csshifts[idx])
		 /(cs_->dah(idx+1)-cs_->dah(idx));

    for ( int logidx =0; logidx<log_->size()-1; logidx++)
    {
	for (int csidx=0; csidx<cs_->size()-1; csidx++)
	{
	    if ( (cs_->dah(csidx) <= log_->dah(logidx)) 
			&& (cs_->dah(csidx+1) > log_->dah(logidx)) )
		logshifts += coeffs[csidx]*log_->dah(logidx) 
			   + csshifts[csidx]-coeffs[csidx]*cs_->dah(csidx);  
	}
	if ( log_->dah(logidx) <= cs_->dah(0))
	    logshifts += 0;
	if ( log_->dah(logidx) > cs_->dah(cs_->size()-1))
	    logshifts += logshifts[logidx-1];
    }
    for ( int idx=0; idx< log_->size()-1; idx++ )
    {
	if ( idx>1 && (mIsUdf(log_->value(idx)) || mIsUdf(logshifts[idx])) )
	    log_->valArr()[idx] = log_->valArr()[idx-1];
	else
	    log_->valArr()[idx]  = log_->value(idx) + logshifts[idx]; 
    }
}

}; //namespace WellTie
