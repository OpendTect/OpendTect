/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		June 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: createlogcube.cc,v 1.2 2011-07-05 09:23:34 cvsbruno Exp $";

#include "createlogcube.h"

#include "seistrc.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "ctxtioobj.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"


LogCubeCreator::LogCubeData::~LogCubeData()
{
    delete seisctio_.ioobj;
}


LogCubeCreator::LogCubeCreator( const Well::Track& t, const Well::D2TModel* d2t)
    : d2t_(d2t)
    , hrg_(false)
    , nrduplicatetrcs_(0)		 
{
    Well::SimpleTrackSampler wtextr( t, d2t );
    if ( !wtextr.execute() )
	pErrMsg( "unable to extract position" );
    wtextr.getBIDs( binids_ );
}


LogCubeCreator::~LogCubeCreator()
{
    deepErase( logdatas_ );
}


void LogCubeCreator::setInput( ObjectSet<LogCubeData>& lcds, int nrdupltrcs )
{
    while ( !lcds.isEmpty() )
	logdatas_ += lcds.remove(0);
    nrduplicatetrcs_ = nrdupltrcs;
}


#define mErrRet(msg) { errmsg_ = msg; return false; }
bool LogCubeCreator::doPrepare( int )
{
    if ( binids_.isEmpty() )
	mErrRet( "No valid position extracted along the track" );

    hrg_ = HorSampling( false );
    for ( int idx=0; idx<binids_.size(); idx++ )
	hrg_.include( binids_[idx] );

    BinID bidvar( nrduplicatetrcs_-1, nrduplicatetrcs_-1 );
    hrg_.stop += bidvar;
    hrg_.start -= bidvar;
    hrg_.snapToSurvey();
    
    return true;
}


bool LogCubeCreator::doWork( od_int64 start, od_int64 stop, int )
{
    if ( SI().zIsTime() && !d2t_ )
	{ errmsg_ = "No depth/time model found"; return false; }

    for ( int idx=start; idx<=stop; idx++ )
    {
	if ( !writeLog2Cube( *logdatas_[idx] ) )
	    { errmsg_ = "One or several logs could not be written"; }
    }
    return true;
}


bool LogCubeCreator::writeLog2Cube( const LogCubeData& lcd ) const
{
    SeisTrcWriter writer( lcd.seisctio_.ioobj );

    SeisTrc trc( SI().zRange(true).nrSteps() );
    trc.info().sampling.step = SI().zStep();

    for ( int idx=0; idx<trc.size(); idx++ )
    {
	float z = trc.info().sampling.atIndex( idx );
	if ( d2t_ ) z = d2t_->getDah( z );
	trc.set( idx, lcd.log_.getValue( z, true ),0 );
    }
    HorSamplingIterator hsit( hrg_ );
    bool succeeded = true;
    BinID bid;
    while ( hsit.next( bid ) )
    {
	trc.info().binid = bid;
	if ( !writer.put(trc) )
	    { pErrMsg( "cannot write new trace" ); succeeded = false; }
    }
    return succeeded;
}


const char* LogCubeCreator::errMsg() const 
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }
