/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltieextractdata.cc,v 1.17 2009-10-15 10:05:55 cvsbert Exp $";

#include "welltieextractdata.h"
#include "welltiegeocalculator.h"

#include "arrayndimpl.h"
#include "interpol1d.h"
#include "datapointset.h"
#include "survinfo.h"

#include "welldata.h"
#include "welltiedata.h"
#include "welllog.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welltrack.h"


namespace WellTie
{

int TrackExtractor::nextStep()
{
    double time = timeintv_.atIndex( nrdone_ );

    Coord3 pos = wd_.track().getPos( wd_.d2TModel()->getDepth(time) );
    pos.z = time;

    const BinID bid = SI().transform( pos );
    if ( !bid.inl && !bid.crl )
    {
	nrdone_++;
       	return Executor::MoreToDo();
    }
    const int d2tsz = wd_.d2TModel()->size();
    if ( time>timeintv_.stop )
	return Executor::Finished();
    DataPointSet::DataRow dr;
    if ( dps_ )
    {
	dr.pos_.set( pos );
	dps_->addRow( dr );
	dps_->dataChanged();
    }
    bidvalset_ += bid;

    nrdone_ ++;
    return Executor::MoreToDo();
}


#define mErrRet(s) { errmsg = s; return; }
LogResampler::LogResampler( Well::Log* newl, const Well::Log& orgl, 
			    const Well::Data* d, WellTie::DataHolder* dh )
    	: Executor("Processing log data") 
	, newlog_(newl)
	, orglog_(orgl)	     
	, wd_(*d)	   
	, nrdone_(0)
	, curidx_(0)
	, isavg_(true) 
	, vals_(0)	       
	, dahs_(0)	       
{
    if ( newlog_ ) newlog_->erase();

    BufferString exnm = "Processing "; exnm += orgl.name(); exnm += " Data"; 
    setName( exnm );

    fillProcLog( orgl );

    BufferString errmsg;
    errmsg += "no valid "; errmsg += orgl.name();  errmsg += " log selected";

    if ( dh )
    {
	if ( !dh->geoCalc()->isValidLogData( val_ ) ) mErrRet(errmsg);
	dh->geoCalc()->interpolateLogData( dah_, orgl.dahStep(true), true );
	dh->geoCalc()->interpolateLogData( val_, orgl.dahStep(true), false );
    }
}


LogResampler::~LogResampler() 
{
    delete vals_; delete dahs_;
}


void LogResampler::fillProcLog( const Well::Log& log )
{
    for ( int idx=0; idx<log.size(); idx++ )
    {
	val_ += log.valArr()[idx];
	dah_ += log.dah(idx);
    }
}


int LogResampler::nextStep()
{
    float curtime = timeintv_.atIndex( nrdone_ );
    if ( curtime >= timeintv_.stop || nrdone_ >= timeintv_.nrSteps() )
	return Executor::Finished();
   
    float curdah = wd_.d2TModel()->getDepth( curtime );
    curidx_ = orglog_.indexOf( curdah );
    if ( curidx_ < 0 ) 
	curidx_ = 0;
    
    float curval = 0;
    if ( curdah < dah_[0] )
	curval = val_[0];
    else if ( curdah > dah_[dah_.size()-1] )
	curval = val_[dah_.size()-1];
    else if ( curidx_>1 && curidx_<dah_.size()-2 && isavg_ )
	curval += ( val_[curidx_+1] + val_[curidx_-1] + val_[curidx_] )/3;
    else
	curval = val_[curidx_];

    if ( newlog_ )
	newlog_->addValue( curdah, curval );
    if ( nrdone_ < dahs_->info().getSize(0) )
    {
	dahs_->set( nrdone_, curdah );
	vals_->set( nrdone_, curval );
    }

    nrdone_++;
    return Executor::MoreToDo();
}

void LogResampler::setTimeIntv( const StepInterval<float>& itv )
{
    timeintv_ = itv;
    vals_ = new Array1DImpl<float>( itv.nrSteps() );
    dahs_ = new Array1DImpl<float>( itv.nrSteps() );
}

}; //namespace WellTie
