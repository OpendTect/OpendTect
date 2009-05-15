/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiecshot.cc,v 1.3 2009-05-15 12:42:48 cvsbruno Exp $";

#include "welltiecshot.h"

#include "idxable.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"
#include "welltiegeocalculator.h"


WellTieCSCorr::WellTieCSCorr( Well::Data& d, const WellTieParams& pms )
	: log_(new Well::Log(*d.logs().getLog(pms.getSetup().vellognm_)))
	, cs_(d.checkShotModel())
{
    if ( !cs_ )
	return;
    if ( !cs_->size() )
	return;
    
    TypeSet<float> newcsvals; 

    WellTieGeoCalculator geocalc( &pms, &d );
    setCSToLogScale( newcsvals, pms.getUnits().velFactor(), geocalc );
    fitCS( newcsvals );
    BufferString corr = "Corrected ";
    corr += log_->name();
    log_->setName( pms.getSetup().corrvellognm_ );
    d.logs().add( log_ );
}


void WellTieCSCorr::setCSToLogScale( TypeSet<float>& cstolog, double velfactor,
				      WellTieGeoCalculator& geocalc )
{
    TypeSet<float> dpt, csvals;
    for ( int idx=0; idx<cs_->size(); idx++ )
    {
	dpt     += cs_->dah(idx);
	csvals += cs_->value(idx);
    }
    geocalc.TWT2Vel( csvals, dpt, cstolog, true );
}


void WellTieCSCorr::fitCS( const TypeSet<float>& csvals ) 
{
    TypeSet<float> logvaldah, coeffs, logshifts, csshifts;
   
    /* 
    for ( int idx=0; idx<log_.size(); idx++ )
    {
	if ( idx>1 && (mIsUdf(log_.value(idx))) )
	    logvaldah += log_.valArr()[idx-1];
	else
	    logvaldah +=  log_.value(idx); 
	ctrlspls += idx;
    }

    IdxAble::callibrateArray<float*>( logvaldah.arr(), logvaldah.size(),
		    csvals.arr(), ctrlspls, csvals.size(), false,
		    logshifts.arr() );

    for ( int idx=0; idx<log_.size(); idx++ )
    {
	if ( idx>1 && (mIsUdf(log_.value(idx)) || mIsUdf(logshifts[idx])) )
	    log_.valArr()[idx] = log_.valArr()[idx-1];
	else
	    log_.valArr()[idx]  = logshifts[idx]; 
    }
*/

    for ( int idx=0; idx<cs_->size(); idx++)
    {
	logvaldah += log_->getValue(cs_->dah(idx)); 
	csshifts += csvals[idx]-logvaldah[idx];
    }

    for ( int idx=0; idx<cs_->size()-1; idx++)
	coeffs +=  (csshifts[idx+1]-csshifts[idx])
	    	  /(cs_->dah(idx+1)-cs_->dah(idx));

    for ( int logidx =0; logidx<log_->size()-1; logidx++)
    {
	for (int csidx=0; csidx<cs_->size(); csidx++)
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
    
    for ( int idx=0; idx< log_->size(); idx++ )
    {
	if ( idx>1 && (mIsUdf(log_->value(idx)) || mIsUdf(logshifts[idx])) )
	    log_->valArr()[idx] = log_->valArr()[idx-1];
	else
	    log_->valArr()[idx]  = log_->value(idx) + logshifts[idx]; 
    }
}
