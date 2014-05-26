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
#include "seisbuf.h"
#include "seistrcprop.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllogset.h"
#include "wellman.h"


#define mErrRet(msg,act)\
{ \
    if ( !errmsg_.isEmpty() ) \
	errmsg_.addNewLine(); \
    \
    errmsg_.add( msg ); \
    act; \
}



bool LogCubeCreator::LogCubeData::makeWriteReady()
{
    if ( outfnm_.isEmpty() )
	mErrRet( "Internal: no output file specified", return false )

    if ( !mkIOObj() )
	mErrRet( "cannot write new trace to disk", return false )

    return true;
}


bool LogCubeCreator::LogCubeData::mkIOObj()
{
    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.forread = false;
    ctxt.deftransl = CBVSSeisTrcTranslator::translKey();

    IOM().to( ctxt.getSelKey() );
    CtxtIOObj ctio( ctxt );
    ctio.setName( outfnm_ );
    IOM().getEntry( ctio );
    if ( !ctio.ioobj )
	return false;

    if ( !IOM().commitChanges(*ctio.ioobj) )
	return false;

    seisioobj_ = ctio.ioobj;
    return true;
}


bool LogCubeCreator::LogCubeData::doWrite( const SeisTrcBuf& trcs ) const
{
    if ( !seisioobj_ )
	mErrRet( "Internal: no ioobj specified", return false )

    SeisTrcWriter writer( seisioobj_ );
    for ( int itrc=0; itrc<trcs.size(); itrc++ )
    {
	if ( !writer.put(*trcs.get(itrc)) )
	{
	    delete seisioobj_;
	    mErrRet( "cannot write new trace", return false );
	}
    }

    delete seisioobj_;
    return true;
}


const char* LogCubeCreator::LogCubeData::errMsg() const
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }



LogCubeCreator::WellData::WellData( const MultiID& wid )
    : wd_(Well::MGR().get(wid))
    , hrg_(HorSampling(false))
{
    if ( !wd_ )
    { errmsg_.set( "Cannot open well" ); return; }

    Well::SimpleTrackSampler wtextr( wd_->track(), wd_->d2TModel(), true, true);
    wtextr.setSampling( SI().zRange(true) );
    if ( !wtextr.execute() )
    { pErrMsg( "unable to extract position" ); }

    wtextr.getBIDs( binids_ );
}


LogCubeCreator::WellData::~WellData()
{
    deepErase( trcs_ );
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
    deepErase( welldata_ );
    deepErase( logdatas_ );
}

#define mErrRetwWell(msg,wllnm,act)\
{\
    mErrRet(msg,;) \
    errmsg_.add( " for well " ).add( wllnm ); \
    act; \
}


#define mGetWellName(idx) ( BufferString(welldata_[idx]->wd_->name()) )


bool LogCubeCreator::init( const BufferStringSet& lognms,
			   const TypeSet<MultiID>& wllids )
{
    for ( int ilog=0; ilog<lognms.size(); ilog++ )
    {
	if ( !lognms.validIdx(ilog) )
	    continue;

	const BufferString& lognm = lognms.get( ilog );
	if ( lognm.isEmpty() )
	    continue;

	logdatas_ += new LogCubeData( lognm, 0 );
    }

    TypeSet<int> goodwells;
    for ( int iwell=0; iwell<wllids.size(); iwell++ )
    {
	const MultiID& wllid = wllids[iwell];
	WellData* welldata = new WellData( wllid );
	const FixedString errmsg( welldata->errMsg() );
	if ( !errmsg.isEmpty() )
	{
	    BufferString wllnm;
	    if ( welldata->wd_ )
		wllnm = welldata->wd_->name();

	    mErrRetwWell( errmsg, wllnm, continue )
	}

	welldata_ += welldata;
	goodwells += iwell;
    }

    return true;
}


bool LogCubeCreator::setOutputNm( const char* suffix, bool withwllnm )
{
    if ( withwllnm && welldata_.size() > 1 )
	mErrRet( "Cannot append well name with multiple wells", return false; )

    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.deftransl = "3D";
    ctxt.forread = false;
    ctxt.deftransl = CBVSSeisTrcTranslator::translKey();

    BufferString wellnmsuffix;
    if ( withwllnm )
    {
	if ( !welldata_.validIdx(0) || !welldata_[0]->wd_ )
	    mErrRet( "Internal: no well name found", return false)

	wellnmsuffix.set( "from well " ).add( welldata_[0]->wd_->name() );
    }

    for ( int ilog=0; ilog<logdatas_.size(); ilog++ )
    {
	BufferString& fnm = logdatas_[ilog]->outfnm_;
	if ( suffix )
	    fnm.addSpace().add( suffix );

	if ( withwllnm )
	    fnm.addSpace().add( wellnmsuffix );

	const IOObj* presentobj = IOM().getLocal( fnm.buf(),
						  ctxt.trgroup->userName() );
	if ( !presentobj )
	    continue;

	BufferString msg( "Volume: '", fnm, "' is already present");
	if ( ctxt.deftransl != presentobj->translator() )
	{
	    fnm.setEmpty();
	    msg.add( " as another type\nand won't be created");
	}

	mErrRet( msg, continue )
    }

    return errmsg_.isEmpty();
}


bool LogCubeCreator::doPrepare( int )
{
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

	if ( !makeLogTraces(idx) )
	{
	    if ( !errmsg_.isEmpty() )
	    {
		const BufferString errmsg( msg, errmsg_ );
		errmsg_.set( errmsg );
	    }
	}

	addToNrDone( 1 );
    }

    return true;
}


bool LogCubeCreator::doFinish( bool success )
{
    const BufferString msg( "One or several log cubes could not be computed" );

    SeisTrcBuf trcsbufsout( true );
    for ( int ilog=0; ilog<logdatas_.size(); ilog++ )
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

	LogCubeData& logdata = *logdatas_[ilog];
	if ( trcsbufsout.isEmpty() )
	{
	    BufferString errmsg( "No data for log ", logdata.lognm_ );
	    errmsg.add( ": no cube created" );
	    mErrRet( errmsg, continue )
	}

	if ( !logdata.makeWriteReady() )
	{
	    if ( errmsg_.isEmpty() ) mErrRet( msg, ; )
	    mErrRet( FixedString(logdata.errMsg()), continue )
	}

	trcsbufsout.sortForWrite( false );
	if ( !logdata.doWrite(trcsbufsout) )
	{
	    if ( errmsg_.isEmpty() ) mErrRet( msg, ; )
	    mErrRet( FixedString(logdata.errMsg()), ; )
	}
    }

    trcsbufsout.erase();

    return errmsg_.isEmpty();
}


bool LogCubeCreator::makeLogTraces( int iwll )
{
    if ( !welldata_.validIdx(iwll) || !welldata_[iwll]->wd_ )
	mErrRet( "Internal: No well data", return false );

    if ( logdatas_.isEmpty() )
	mErrRet( "Internal: No log cube data", return false );

    const Well::Data& wd = *welldata_[iwll]->wd_;
    const BufferString wllnm = mGetWellName( iwll );

    if ( SI().zIsTime() && !wd.haveD2TModel() )
	mErrRetwWell( "No depth/time model found", wllnm, return false );

    BufferStringSet lognms;
    getLogNames( lognms );
    Well::LogSampler logsamp( wd, extractparams_, lognms );
    if ( !logsamp.execute() )
	mErrRetwWell( logsamp.errMsg(), wllnm, return false )

    const SamplingData<float> sampling( SI().zRange( true ) );
    const int trcsz = SI().zRange( true ).nrSteps() + 1;
    SeisTrc undeftrc( trcsz );
    undeftrc.info().sampling = sampling;
    for ( int idx=0; idx<undeftrc.size(); idx++ )
	undeftrc.set( idx, mUdf(float), 0 );

    ObjectSet<SeisTrc> logtrcs;
    BoolTypeSet logispresent;
    for ( int ilog=0; ilog<lognms.size(); ilog++ )
    {
	logtrcs += new SeisTrc( undeftrc );
	if ( !logtrcs.validIdx(ilog) )
	    mErrRetwWell( "Cannot allocate memory for output log", wllnm,
			  return false );

	logtrcs[ilog]->info().sampling = sampling;
	welldata_[iwll]->trcs_ += new SeisTrcBuf( true );
	logispresent += wd.logs().getLog( lognms.get(ilog) );
    }

    StepInterval<float> logzrg( logsamp.zRange().start, logsamp.zRange().stop,
				extractparams_.zstep_ );
    const int ns = logsamp.nrZSamples();
    const int nrlogs = logdatas_.size();
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

    const BinID bidvar( nrduplicatetrcs_, nrduplicatetrcs_ );
    const od_int64 sqstep = nrduplicatetrcs_ * nrduplicatetrcs_;
    const TypeSet<BinID>& trackbinids = welldata_[iwll]->binids_;
    for ( int itrckpt=0; itrckpt<trackbinids.size(); itrckpt++ )
    {
	const BinID& trackpos = trackbinids[itrckpt];
	for ( int ilog=0; ilog<nrlogs; ilog++ )
	{
	    logvals[ilog] = logtrcs.validIdx(ilog) && logispresent[ilog] ?
			    logtrcs[ilog]->get( itrckpt, 0 ) : mUdf(float);
	}

	HorSampling hrg(false);
	hrg.start = trackpos - bidvar;
	hrg.stop = trackpos + bidvar;
	hrg.snapToSurvey();

	HorSamplingIterator hsit( hrg );
	BinID bid;
	while ( hsit.next(bid) )
	{
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
			trcsbufout.last()->info().binid = bid;
			trcpos = trcsbufout.size() - 1;
		    }

		    const float logval = logvals[ilog];
		    if ( mIsUdf(logval) )
			continue;

		    trcsbufout.get( trcpos )->set( itrckpt, logval, 0 );
		}
	    }
	}
    }

    deepErase( logtrcs );

    return true;
}


void LogCubeCreator::getLogNames( BufferStringSet& lognms ) const
{
    for ( int ilc=0; ilc<logdatas_.size(); ilc++ )
	lognms.add( logdatas_[ilc]->lognm_ );
}


void LogCubeCreator::addUniqueTrace( const SeisTrc& trc,
				     SeisTrcBuf& trcs ) const
{
    const BinID& bid = trc.info().binid;
    const int pos = trcs.find( bid );
    if ( pos < 0 )
    {
	trcs.add( new SeisTrc(trc) );
	return;
    }

    SeisTrcPropChg stckr( *trcs.get( pos ) );
    stckr.stack( trc );
}


bool LogCubeCreator::writeLog2Cube( const LogCubeData& lcd ) const
{ return false; }


const char* LogCubeCreator::errMsg() const
{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }
