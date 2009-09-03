/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiecshot.cc,v 1.9 2009-09-03 09:41:40 cvsbruno Exp $";

#include "welltiecshot.h"

#include "idxable.h"
#include "welldata.h"
#include "welllogset.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"
#include "welltiegeocalculator.h"

namespace WellTie
{

CheckShotCorr::CheckShotCorr( Well::Data& d, const WellTie::Params& pms )
	: log_(new Well::Log(*d.logs().getLog(pms.getSetup().vellognm_)))
	, cs_(d.checkShotModel())
{
    if ( !cs_ || !cs_->size() )
	return;
    
    TypeSet<float> newcsvals; 

    WellTie::GeoCalculator geocalc( &pms, &d );
    setCSToLogScale( newcsvals, pms.getUnits().velFactor(), geocalc );
    calibrateLogToCS( newcsvals, geocalc );
    log_->setName( pms.getSetup().corrvellognm_ );
    d.logs().add( log_ );
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
    const int logsz = log_->size();
    TypeSet<int> ctrlsamples;		ctrlsamples.setSize( csvals.size() );
    TypeSet<float> calibratedpts;	calibratedpts.setSize( logsz );

    for ( int idx=0; idx<csvals.size(); idx++ )
    {
	int dahidx = log_->indexOf( cs_->dah(idx) );
	if ( dahidx == -1 || dahidx >= logsz )
	{
	    log_->addValue( cs_->dah(idx), csvals[idx] );
	    log_->ensureAscZ();
	    continue;
	}
	ctrlsamples += logsz-dahidx;
    }
   
    IdxAble::callibrateArray( log_->valArr(), logsz,
		    csvals.arr(), ctrlsamples.arr(), csvals.size(), false,
		    calibratedpts.arr() );
    
    memcpy( log_->valArr(), calibratedpts.arr(), logsz*sizeof(float) );
}

}; //namespace WellTie
