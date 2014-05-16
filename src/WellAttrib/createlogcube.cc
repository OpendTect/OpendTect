/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		June 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "createlogcube.h"

#include "ioman.h"
#include "seiscbvs.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "stattype.h"
#include "survinfo.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltrack.h"


const char* LogCubeCreator::LogCubeData::errMsg() const
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }


LogCubeCreator::LogCubeData::~LogCubeData()
{
    if ( seisioobj_ ) delete seisioobj_;
}



LogCubeCreator::WellData::WellData( const MultiID& wid )
    : wd_(Well::MGR().get(wid))
    , hrg_(HorSampling(false))
{
    if ( !wd_ )
    { errmsg_.set( "Cannot open well" ); return; }

    Well::SimpleTrackSampler wtextr( wd_->track(), wd_->d2TModel() );
    if ( !wtextr.execute() )
    { pErrMsg( "unable to extract position" ); }

    wtextr.getBIDs( binids_ );
}


const char* LogCubeCreator::WellData::errMsg() const
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }



LogCubeCreator::LogCubeCreator( const BufferStringSet& lognms,
				const MultiID& wllid,
				const Well::ExtractParams& pars, int nrtrcs )
    : extractparams_(pars)
    , nrduplicatetrcs_(nrtrcs)
    , domerge_(false)
{
    TypeSet<MultiID> wllids;
    wllids += wllid;
    init( lognms, wllids );
}


LogCubeCreator::LogCubeCreator( const BufferStringSet& lognms,
				const TypeSet<MultiID>& wllids,
				const Well::ExtractParams& pars, int nrtrcs )
    : extractparams_(pars)
    , nrduplicatetrcs_(nrtrcs)
    , domerge_(false)
{
    init( lognms, wllids );
}


LogCubeCreator::~LogCubeCreator()
{
    deepErase( logdatas_ );
    deepErase( welldata_ );
    deepErase( seisioobjs_ );
}


#define mErrRet(msg,wllnm,act)\
{ \
    if ( !errmsg_.isEmpty() ) \
	errmsg_.addNewLine(); \
    \
    errmsg_.add( msg ).add( " for well " ).add( wllnm ); \
    act; \
}


#define mGetWellName(idx) ( welldata_[logdatas_[idx]->iwll_]->wd_->name() )


bool LogCubeCreator::init( const BufferStringSet& lognms,
			   const TypeSet<MultiID>& wllids )
{
    TypeSet<int> goodwells;
    for ( int iwell=0; iwell<wllids.size(); iwell++ )
    {
	const MultiID& wllid = wllids[iwell];
	WellData* welldata = new WellData( wllid );
	if ( !FixedString(welldata->errMsg()).isEmpty() )
	{
	    const BufferString wllnm = welldata->wd_
				     ? BufferString( welldata->wd_->name() )
				     : BufferString( wllid );
	    mErrRet( welldata->errMsg(), wllnm, continue );
	}

	welldata_ += welldata;
	goodwells += iwell;
    }

    for ( int ilog=0; ilog<lognms.size(); ilog++ )
    {
	for ( int iwell=0; iwell<goodwells.size(); iwell++ )
	{
	    LogCubeData* logdata = new LogCubeData( *lognms[ilog],
						    goodwells[iwell] );
	    logdatas_ += logdata;
	}
    }

    return true;
}


bool LogCubeCreator::setOutputNm( const char* suffix, bool withwllnm )
{
    BufferString postfix( suffix );
    postfix.trimBlanks();

    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.deftransl = "3D";
    ctxt.forread = false;
    ctxt.deftransl = CBVSSeisTrcTranslator::translKey();

    for ( int idx=0; idx<nrIterations(); idx++ )
    {
	if ( !logdatas_.validIdx(idx) )
	    continue;

	LogCubeData& logdata = *logdatas_[idx];
	if ( withwllnm )
	    logdata.outfnm_.add( " " ).add( mGetWellName(idx) );

	if ( !postfix.isEmpty() )
	    logdata.outfnm_.add( " " ).add( postfix );

	const IOObj* presentobj = IOM().getLocal( logdata.outfnm_.buf(),
						  ctxt.trgroup->userName() );
	if ( !presentobj )
	    continue;

	BufferString msg( "Volume: '", logdata.outfnm_, "' is already present");
	if ( ctxt.deftransl != presentobj->translator() )
	    msg.add( " as another type\nand won't be created");

	mErrRet( msg, mGetWellName(idx), continue );
    }

    return !errMsg();
}


bool LogCubeCreator::doPrepare( int )
{
    const BinID bidvar( nrduplicatetrcs_-1, nrduplicatetrcs_-1 );
    const BufferString msg( "No valid position extracted along the track" );
    for ( int iwell=0; iwell<welldata_.size(); iwell++ )
    {
	if ( !welldata_.validIdx(iwell) )
	{ pErrMsg( "Invalid well data" ); }

	WellData& welldata = *welldata_[iwell];
	const TypeSet<BinID>& binid = welldata.binids_;
	if ( binid.isEmpty() )
	    mErrRet( msg, welldata.wd_->name(), continue )

	HorSampling& hrg = welldata.hrg_;
	for ( int ipts=0; ipts<binid.size(); ipts++ )
	    hrg.include( binid[ipts] );

	hrg.start -= bidvar;
	hrg.stop += bidvar;
	hrg.snapToSurvey();
    }

    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.forread = false;
    ctxt.deftransl = CBVSSeisTrcTranslator::translKey();

    for ( int idx=0; idx<nrIterations(); idx++ )
    {
	if ( !logdatas_.validIdx(idx) )
	    continue;

	IOM().to( ctxt.getSelKey() );
	CtxtIOObj ctio( ctxt );
	ctio.setName( logdatas_[idx]->outfnm_ );
	IOM().getEntry( ctio );
	if ( !ctio.ioobj )
	    return false;

	IOM().commitChanges( *ctio.ioobj );
	logdatas_[idx]->seisioobj_ = ctio.ioobj;
    }

    extractparams_.zstep_ = SI().zRange( true ).step;
    extractparams_.extractzintime_ = SI().zIsTime();
    extractparams_.snapZRangeToSurvey( true );

    return true;
}


bool LogCubeCreator::doWork( od_int64 start, od_int64 stop, int )
{
    const BufferString msg( "One or several log cubes could not be computed" );
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	if ( !shouldContinue() )
	    return false;

	if ( !logdatas_.validIdx(idx) )
	{ pErrMsg( "Invalid LogCubeData" ); }

	if ( !writeLog2Cube(*logdatas_[idx]) )
	{
	    if ( errmsg_.isEmpty() )
	    { errmsg_.set( msg ); return false; }
	    else
		mErrRet( msg, mGetWellName(idx), return false );
	}

	addToNrDone( 1 );
    }
    return true;
}


bool LogCubeCreator::writeLog2Cube( const LogCubeData& lcd ) const
{
    if ( lcd.lognm_.isEmpty() || lcd.outfnm_.isEmpty() )
	mErrRet( "Internal: No log name", "", return false );

    const int wll = lcd.iwll_;
    if ( !welldata_.validIdx(wll) )
	mErrRet( "Internal: No well data", "", return false );

    if ( !lcd.seisioobj_ || lcd.seisioobj_->isBad() )
	mErrRet( "Internal: No seismic object", "", return false );

    const Well::Data& wd = *welldata_[wll]->wd_;
    const BufferString wllnm = wd.name();
    BufferStringSet lognms; lognms.add( lcd.lognm_ );
    if ( SI().zIsTime() && !wd.haveD2TModel() )
	mErrRet( "No depth/time model found", wllnm, return false );

    Well::LogSampler logsamp( wd, extractparams_, lognms );
    if ( !logsamp.executeParallel(false) )
	mErrRet( logsamp.errMsg(), wllnm, return false )

    const int ns = logsamp.nrZSamples();
    StepInterval<float> zrg( logsamp.zRange().start, logsamp.zRange().stop,
			     extractparams_.zstep_ );
    SeisTrc trc( SI().zRange(true).nrSteps() + 1 );
    trc.info().sampling = SI().zRange(true);
    for ( int idztrc=0; idztrc<trc.size(); idztrc++ )
    {
	const float depth = trc.info().sampling.atIndex(idztrc);
	float val = mUdf(float);
	if ( zrg.includes(depth,true) )
	{
	    const int idz = zrg.getIndex( depth );
	    if ( idz >=0 && idz < ns )
		val = logsamp.getLogVal( 0, idz );
	}

	trc.set( idztrc, val, 0 );
    }

    if ( domerge_ )
	return true;

    SeisTrcWriter writer( lcd.seisioobj_ );
    const HorSampling& hrg = welldata_[wll]->hrg_;
    HorSamplingIterator hsit( hrg );
    BinID bid;
    while ( hsit.next(bid) )
    {
	trc.info().binid = bid;
	if ( !writer.put(trc) )
	    mErrRet( "cannot write new trace", wllnm, return false );
    }

    return true;
}


const char* LogCubeCreator::errMsg() const
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }
