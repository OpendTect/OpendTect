/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		June 2011
________________________________________________________________________

-*/

#include "createlogcube.h"

#include "dbman.h"
#include "dbdir.h"
#include "seiscbvs.h"
#include "seisbuf.h"
#include "seistrcprop.h"
#include "seisstorer.h"
#include "survinfo.h"
#include "trckeysampling.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllogset.h"
#include "welldata.h"
#include "wellmanager.h"


#define mErrRet(msg,new,act)\
{ \
    if ( new ) \
	errmsg_ = msg; \
    else \
	errmsg_.appendPhrase( msg ); \
\
    act; \
}


bool LogCubeCreator::LogCube::makeStoreReady()
{
    if ( fnm_.isEmpty() )
	mErrRet( mINTERNAL("no output file specified"), true, return false )

    if ( !mkIOObj() )
	mErrRet( tr("Cannot write new trace to disk"), true, return false )

    return true;
}


bool LogCubeCreator::LogCube::mkIOObj()
{
    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.forread_ = false;

    CtxtIOObj ctio( ctxt );
    ctio.setName( fnm_ );
    DBM().getEntry( ctio );
    if ( !ctio.ioobj_ )
	return false;

    seisioobj_ = ctio.ioobj_;
    return true;
}


bool LogCubeCreator::LogCube::doStore( const SeisTrcBuf& trcs ) const
{
    if ( !seisioobj_ )
	mErrRet( mINTERNAL("no ioobj specified"), true, return false )

    Seis::Storer storer( *seisioobj_ );
    delete seisioobj_;

    for ( int itrc=0; itrc<trcs.size(); itrc++ )
    {
	auto uirv = storer.put( *trcs.get(itrc) );
	if ( !uirv.isOK() )
	    { errmsg_ = uirv; return false; }
    }

    errmsg_ = storer.close();
    return errmsg_.isEmpty();
}



LogCubeCreator::WellData::WellData( const DBKey& wid )
{
    uiRetVal uirv;
    wd_ = Well::MGR().fetch( wid, Well::LoadReqs(), uirv );
    if ( !wd_ )
	mErrRet( uirv, true, return )

    Well::SimpleTrackSampler wtextr( wd_->track(), wd_->d2TModel(), true, true);
    wtextr.setSampling( SI().zRange() );
    if ( !wtextr.execute() )
	mErrRet( mINTERNAL("unable to extract track positions"), true, return )

    wtextr.getBIDs( binidsalongtrack_ );
    if ( binidsalongtrack_.isEmpty() )
	mErrRet( tr("Cannot use a well outside of the survey area"), true, ; )
}


LogCubeCreator::WellData::~WellData()
{
    deepErase( trcs_ );
}



LogCubeCreator::LogCubeCreator( const BufferStringSet& lognms,
				const DBKey& wllid,
				const Well::ExtractParams& pars, int nrtrcs )
    : extractparams_(pars)
    , stepout_(nrtrcs)
{
    DBKeySet wllids;
    wllids += wllid;
    init( lognms, wllids );
}


LogCubeCreator::LogCubeCreator( const BufferStringSet& lognms,
				const DBKeySet& wllids,
				const Well::ExtractParams& pars, int nrtrcs )
    : extractparams_(pars)
    , stepout_(nrtrcs)
{
    init( lognms, wllids );
}


LogCubeCreator::~LogCubeCreator()
{
    deepErase( welldata_ );
    deepErase( logcubes_ );
}


bool LogCubeCreator::init( const BufferStringSet& lognms,
			   const DBKeySet& wllids )
{
    for ( int ilog=0; ilog<lognms.size(); ilog++ )
    {
	if ( !lognms.validIdx(ilog) )
	    continue;

	const BufferString& lognm = lognms.get( ilog );
	if ( lognm.isEmpty() )
	    continue;

	logcubes_ += new LogCube( lognm );
    }

    TypeSet<int> goodwells;
    for ( int iwell=0; iwell<wllids.size(); iwell++ )
    {
	const DBKey& wllid = wllids[iwell];
	WellData* welldata = new WellData( wllid );
	if ( !welldata->isOK() )
	{
	    BufferString wllnm;
	    if ( welldata->wd_ )
		wllnm = welldata->wd_->name();

	    const uiString msg = toUiString( "%1 @ %2" )
				     .arg( welldata->errMsg() ).arg( wllnm );
	    mErrRet( msg, errmsg_.isEmpty(), continue )
	}

	welldata_ += welldata;
	goodwells += iwell;
    }

    return !welldata_.isEmpty();
}


#define mCheckInit(msg) \
{ \
    if ( welldata_.isEmpty() ) \
	mErrRet( msg, errmsg_.isEmpty(), return false; ) \
}


bool LogCubeCreator::setOutputNm( const char* suffix, bool withwllnm )
{
    mCheckInit( tr("No wells to process") )

    errmsg_.setEmpty();
    uiString msg;
    if ( withwllnm && welldata_.size() > 1 )
    {
	msg = tr("Cannot append well name with multiple wells");
	mErrRet( msg, true, return false )
    }

    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.forread_ = false;

    BufferString wellnmsuffix;
    if ( withwllnm )
    {
	if ( !welldata_.validIdx(0) || !welldata_[0]->wd_ )
	{
	    msg = mINTERNAL( "no well name found" );
	    mErrRet( msg, true, return false )
	}

	wellnmsuffix.set( "from well " ).add( welldata_[0]->wd_->name() );
    }

    ConstRefMan<DBDir> dbdir = DBM().fetchDir( ctxt );
    if ( !dbdir )
    {
	msg = tr("Database in bad state");
	mErrRet( msg, true, return false )
    }

    for ( int ilog=0; ilog<logcubes_.size(); ilog++ )
    {
	BufferString& fnm = logcubes_[ilog]->fnm_;
	if ( suffix )
	    fnm.addSpace().add( suffix );

	if ( withwllnm )
	    fnm.addSpace().add( wellnmsuffix );

	const IOObj* presentobj = dbdir->getEntryByName( fnm.buf(),
						  ctxt.translatorGroupName() );
	if ( presentobj )
	{
	    msg = tr("Volume: '%1' is already present").arg( fnm );
	    mErrRet( msg, errmsg_.isEmpty(), continue )
	}
    }

    return errmsg_.isEmpty();
}


void LogCubeCreator::getOutputNames( BufferStringSet& names ) const
{
    for ( int idx=0; idx<logcubes_.size(); idx++ )
	names.add( logcubes_[idx]->fnm_ );
}


bool LogCubeCreator::doPrepare( int )
{
    extractparams_.zstep_ = SI().zStep();
    extractparams_.extractzintime_ = SI().zIsTime();
    extractparams_.snapZRangeToSurvey( true );

    mCheckInit( tr("No wells to process") )
    errmsg_.setEmpty();

    return true;
}


bool LogCubeCreator::doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	if ( !makeLogTraces(idx) )
	    continue;

	addToNrDone( 1 );
    }

    return true;
}

#define mAppendHdr( hdrmsg, act ) \
{ \
    errmsg = errmsg_; \
    errmsg_ = hdrmsg; \
    errmsg_.appendPhrase( errmsg ); \
    act; \
}

bool LogCubeCreator::doFinish( bool success )
{
    const uiString hdrmsg= tr("One or several log cubes could not be computed");
    uiString errmsg;

    if ( !nrDone() || !errmsg_.isEmpty() )
	mAppendHdr( hdrmsg, return false )

    SeisTrcBuf trcsbufsout( true );
    for ( int ilog=0; ilog<logcubes_.size(); ilog++ )
    {
	trcsbufsout.erase();
	for ( int iwell=0; iwell<welldata_.size(); iwell++ )
	{
	    if ( !welldata_.validIdx(iwell) )
		continue;

	    if ( !welldata_[iwell]->trcs_.validIdx(ilog) )
		continue;

	    const SeisTrcBuf& welltrcs = *welldata_[iwell]->trcs_[ilog];
	    for ( int itrc=0; itrc<welltrcs.size(); itrc++ )
		addUniqueTrace( *welltrcs.get(itrc), trcsbufsout );
	}

	LogCube& logcube = *logcubes_[ilog];
	if ( trcsbufsout.isEmpty() )
	{
	    const uiString msg = tr("No data for log %1: no cube created")
				     .arg( logcube.lognm_ );
	    mErrRet( msg, errmsg_.isEmpty(), continue )
	}

	if ( !logcube.makeStoreReady() )
	    mErrRet( logcube.errMsg(), errmsg_.isEmpty(), continue )

	trcsbufsout.sortForWrite( false );
	if ( !logcube.doStore(trcsbufsout) )
	    mErrRet( logcube.errMsg(), errmsg_.isEmpty(), ; )
    }

    trcsbufsout.erase();

    if ( !errmsg.isEmpty() )
	mAppendHdr( hdrmsg, return false )

    return true;
}


bool LogCubeCreator::makeLogTraces( int iwll )
{
    uiString msg;

    if ( !welldata_.validIdx(iwll) || !welldata_[iwll]->wd_ )
    {
	msg = mINTERNAL( "No well data at iteration " ).withNumber( iwll );
	mErrRet( msg, errmsg_.isEmpty(), return false )
    }

    ConstRefMan<Well::Data> wd = welldata_[iwll]->wd_;
    const BufferString wllnm = wd->name();
    if ( logcubes_.isEmpty() )
    {
	msg = mINTERNAL( BufferString("No log cube data for well ",wllnm) );
	mErrRet( msg, errmsg_.isEmpty(), return false )
    }

    if ( SI().zIsTime() && !wd->haveD2TModel() )
    {
	msg = tr("No depth/time model found for well %1").arg( wllnm );
	mErrRet( msg, errmsg_.isEmpty(), return false )
    }

    BufferStringSet lognms;
    getLogNames( lognms );
    Well::LogSampler logsamp( *wd, extractparams_, lognms );
    if ( !logsamp.execute() )
    {
	msg = toUiString( "%1 @ %2" ).arg( logsamp.errMsg() ).arg( wllnm );
	mErrRet( msg, errmsg_.isEmpty(), return false )
    }

    const SamplingData<float> sampling( SI().zRange() );
    const int trcsz = SI().zRange().nrSteps() + 1;
    SeisTrc undeftrc( trcsz );
    undeftrc.info().sampling_ = sampling;
    for ( int idx=0; idx<undeftrc.size(); idx++ )
	undeftrc.set( idx, mUdf(float), 0 );

    ObjectSet<SeisTrc> logtrcs;
    BoolTypeSet logispresent;
    for ( int ilog=0; ilog<lognms.size(); ilog++ )
    {
	logtrcs += new SeisTrc( undeftrc );
	if ( !logtrcs.validIdx(ilog) )
	{
	    mErrRet( uiStrings::phrCannotAllocateMemory(), errmsg_.isEmpty(),
		     return false )
	}

	logtrcs[ilog]->info().sampling_ = sampling;
	welldata_[iwll]->trcs_ += new SeisTrcBuf( true );
	logispresent += wd->logs().getLogByName( lognms.get(ilog) ).ptr();
    }

    StepInterval<float> logzrg( logsamp.zRange().start, logsamp.zRange().stop,
				extractparams_.zstep_ );
    const int ns = logsamp.nrZSamples();
    const int nrlogs = logcubes_.size();
    TypeSet<float> logvals;
    logvals.setSize( nrlogs );
    for ( int idztrc=0; idztrc<trcsz; idztrc++ )
    {
	const float depth = sampling.atIndex( idztrc );
	logvals.setAll( mUdf(float) );
	if ( logzrg.includes(depth,true) )
	{
	    const int idz = logzrg.getIndex( depth );
	    if ( idz <0 || idz > ns )
		continue;

	    for ( int ilog=0; ilog<lognms.size(); ilog++ )
		logvals[ilog] = logsamp.getLogVal( ilog, idz );
	}

	for ( int ilog=0; ilog<lognms.size(); ilog++ )
	{
	    if ( logispresent[ilog] )
		logtrcs[ilog]->set( idztrc, logvals[ilog], 0 );
	}
    }

    const BinID bidvar( stepout_, stepout_);
    const od_int64 sqstep = stepout_* stepout_;
    const TypeSet<BinID>& trackbinids = welldata_[iwll]->binidsalongtrack_;
    for ( int itrckpt=0; itrckpt<trackbinids.size(); itrckpt++ )
    {
	const BinID& trackpos = trackbinids[itrckpt];
	for ( int ilog=0; ilog<nrlogs; ilog++ )
	{
	    logvals[ilog] = logtrcs.validIdx(ilog) && logispresent[ilog] ?
			    logtrcs[ilog]->get( itrckpt, 0 ) : mUdf(float);
	}

	TrcKeySampling hrg( false );
	hrg.start_ = trackpos - bidvar;
	hrg.stop_ = trackpos + bidvar;
	hrg.snapToSurvey();

	TrcKeySamplingIterator hsit( hrg );
	do
	{
	    const BinID bid( hsit.curBinID() );
	    if ( bid.sqDistTo(trackpos) <= sqstep )
	    {
		for ( int ilog=0; ilog<nrlogs; ilog++ )
		{
		    if ( !logispresent[ilog] )
			continue;

		    SeisTrcBuf& trcsbufout = *welldata_[iwll]->trcs_[ilog];
		    int trcpos = trcsbufout.find( bid );
		    if ( trcpos < 0 )
		    {
			trcsbufout.add( new SeisTrc(undeftrc) );
			trcsbufout.last()->info().setPos( bid );
			trcpos = trcsbufout.size() - 1;
		    }

		    const float logval = logvals[ilog];
		    if ( mIsUdf(logval) )
			continue;

		    trcsbufout.get( trcpos )->set( itrckpt, logval, 0 );
		}
	    }
	} while ( hsit.next() );
    }

    deepErase( logtrcs );

    return true;
}


void LogCubeCreator::getLogNames( BufferStringSet& lognms ) const
{
    for ( int ilc=0; ilc<logcubes_.size(); ilc++ )
	lognms.add( logcubes_[ilc]->lognm_ );
}


void LogCubeCreator::addUniqueTrace( const SeisTrc& trc,
				     SeisTrcBuf& trcs ) const
{
    const BinID& bid = trc.info().binID();
    const int pos = trcs.find( bid );
    if ( pos < 0 )
    {
	trcs.add( new SeisTrc(trc) );
	return;
    }

    SeisTrcPropChg stckr( *trcs.get( pos ) );
    stckr.stack( trc );
}
