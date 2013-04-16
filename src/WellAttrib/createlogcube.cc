/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		June 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "welltrack.h"
#include "stattype.h"


LogCubeCreator::LogCubeData::~LogCubeData()
{
    delete seisctio_.ioobj;
}


LogCubeCreator::LogCubeCreator( const Well::Data& wd )
    : wd_(wd)
    , hrg_(false)
    , nrduplicatetrcs_(0)		 
{
    Well::SimpleTrackSampler wtextr( wd_.track(), wd_.d2TModel() );
    if ( !wtextr.execute() )
	pErrMsg( "unable to extract position" );
    wtextr.getBIDs( binids_ );
    extractparams_.setFixedRange( SI().zRange(true), SI().zDomain().isTime() );
}


LogCubeCreator::~LogCubeCreator()
{
    deepErase( logdatas_ );
}


void LogCubeCreator::setInput( ObjectSet<LogCubeData>& lcds, int nrdupltrcs )
{
    while ( !lcds.isEmpty() )
	logdatas_ += lcds.removeSingle(0);

    nrduplicatetrcs_ = nrdupltrcs;
}


void LogCubeCreator::setInput( ObjectSet<LogCubeData>& lcds, int nrdupltrcs,
			const Well::ExtractParams& pars )
{
    setInput( lcds, nrdupltrcs );
    extractparams_ = pars;
}


#define mErrRet(msg,withwellname)\
{ \
    if ( !errmsg_.isEmpty() ) \
    	errmsg_.add( "\n" ); \
    \
    errmsg_.add( msg ); \
    if ( withwellname ) \
	errmsg_.add( " for " ).add( wd_.name() ); \
    return false; \
}
bool LogCubeCreator::doPrepare( int )
{
    if ( binids_.isEmpty() )
	mErrRet( "No valid position extracted along the track", true )

    hrg_ = HorSampling( false );
    for ( int idx=0; idx<binids_.size(); idx++ )
	hrg_.include( binids_[idx] );

    BinID bidvar( nrduplicatetrcs_-1, nrduplicatetrcs_-1 );
    hrg_.stop += bidvar;
    hrg_.start -= bidvar;
    hrg_.snapToSurvey();

    extractparams_.zstep_ = SI().zRange( true ).step;
    extractparams_.extractzintime_ = SI().zIsTime();
    extractparams_.snapZRangeToSurvey( true );
    
    return true;
}


bool LogCubeCreator::doWork( od_int64 start, od_int64 stop, int )
{
    if ( SI().zIsTime() && !wd_.haveD2TModel() )
    {
	errmsg_.setEmpty();
	BufferString errmsg = "No depth/time model found";
	errmsg += "\n";
	errmsg += "No log cubes created";
	mErrRet( errmsg, true )
    }

    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	if ( !shouldContinue() )
	    return false;

	if ( !writeLog2Cube(*logdatas_[idx]) )
	{
	    const bool errmsgwithwellnm = errmsg_.isEmpty();
	    mErrRet( "One or several log cubes could not be computed",
		     errmsgwithwellnm )
	}

	addToNrDone( 1 );
    }
    return true;
}


bool LogCubeCreator::writeLog2Cube( const LogCubeData& lcd ) const
{
    if ( !lcd.seisctio_.ioobj )
	return false;

    SeisTrc trc( SI().zRange(true).nrSteps() + 1 );
    trc.info().sampling = SI().zRange(true);

    BufferStringSet lognms; lognms.add( lcd.lognm_ );
    Well::LogSampler logsamp( wd_, extractparams_, lognms );
    if ( !logsamp.execute(false) )
	mErrRet( logsamp.errMsg(), true )

    StepInterval<float> zrg = logsamp.zRange();
    zrg.step = extractparams_.zstep_;
    for ( int idztrc=0; idztrc<trc.size(); idztrc++ )
    {
	const float depth = trc.info().sampling.atIndex(idztrc);
	float val = mUdf(float);
	if ( zrg.includes(depth,true) )
	{
	    const int idz = zrg.getIndex( depth );
	    if ( idz >=0 && idz < logsamp.nrZSamples() )
		val = logsamp.getLogVal( 0, idz );
	}

	trc.set( idztrc, val, 0 );
    }

    SeisTrcWriter writer( lcd.seisctio_.ioobj );
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
