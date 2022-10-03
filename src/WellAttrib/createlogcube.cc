/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
#include "welllog.h"
#include "welldata.h"
#include "wellman.h"


#define mErrRet(msg,new,act)\
{ \
    if ( new ) \
	errmsg_ = msg; \
    else \
	errmsg_.append( msg, true ); \
\
    act; \
}


// LogCubeCreator::LogCube

LogCubeCreator::LogCube::LogCube( const char* lognm )
    : lognm_(lognm)
    , fnm_(lognm)
{
}


LogCubeCreator::LogCube::~LogCube()
{
}


bool LogCubeCreator::LogCube::makeWriteReady()
{
    if ( fnm_.isEmpty() )
	mErrRet( tr( "Internal: no output file specified" ), true, return false)

    if ( !mkIOObj() )
	mErrRet( tr( "Cannot write new trace to disk" ), true, return false )

    return true;
}


bool LogCubeCreator::LogCube::mkIOObj()
{
    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.forread_ = false;
    ctxt.deftransl_ = CBVSSeisTrcTranslator::translKey();

    IOM().to( ctxt.getSelKey() );
    CtxtIOObj ctio( ctxt );
    ctio.setName( fnm_ );
    IOM().getEntry( ctio );
    if ( !ctio.ioobj_ )
	return false;

    if ( !IOM().commitChanges(*ctio.ioobj_) )
    {
	errmsg_ = uiStrings::phrCannotWriteDBEntry( ctio.ioobj_->uiName() );
	return false;
    }

    seisioobj_ = ctio.ioobj_;
    return true;
}


bool LogCubeCreator::LogCube::doWrite( const SeisTrcBuf& trcs ) const
{
    if ( !seisioobj_ )
	mErrRet( tr( "Internal: no ioobj specified" ), true, return false )

    const Seis::GeomType gt = Seis::Vol;
    SeisTrcWriter writer( *seisioobj_, &gt );
    for ( int itrc=0; itrc<trcs.size(); itrc++ )
    {
	if ( !writer.put(*trcs.get(itrc)) )
	{
	    delete seisioobj_;
	    mErrRet( tr( "Cannot write new trace" ), true, return false );
	}
    }

    delete seisioobj_;
    return true;
}


// LogCubeCreator::WellData

LogCubeCreator::WellData::WellData( const MultiID& wid )
    : wd_(Well::MGR().get(wid, Well::LoadReqs(Well::Trck, Well::D2T)))
{
    if ( !wd_ )
	mErrRet( tr( "Cannot open well" ), true, return )

    Well::SimpleTrackSampler wtextr( wd_->track(), wd_->d2TModel(), true, true);
    wtextr.setSampling( SI().zRange(true) );
    if ( !wtextr.execute() )
	mErrRet( tr( "Internal: unable to extract track positions" ), true,
		return )

    wtextr.getBIDs( binidsalongtrack_ );
    if ( binidsalongtrack_.isEmpty() )
	mErrRet( tr( "Cannot use a well outside of the survey area" ), true, ; )
}


LogCubeCreator::WellData::~WellData()
{
    deepErase( trcs_ );
}


// LogCubeCreator

LogCubeCreator::LogCubeCreator( const BufferStringSet& lognms,
				const MultiID& wllid,
				const Well::ExtractParams& pars, int nrtrcs )
    : extractparams_(pars)
    , stepout_(nrtrcs)
{
    TypeSet<MultiID> wllids;
    wllids += wllid;
    init( lognms, wllids );
}


LogCubeCreator::LogCubeCreator( const BufferStringSet& lognms,
				const TypeSet<MultiID>& wllids,
				const Well::ExtractParams& pars, int nrtrcs )
    : extractparams_(pars)
    , stepout_(nrtrcs)
{
    init( lognms, wllids );
}


LogCubeCreator::LogCubeCreator(const BufferStringSet& lognms,
			       const Well::LogSet& logset, const MultiID& wllid,
			       const Well::ExtractParams& pars, int nrtrcs )
    : extractparams_(pars)
    , stepout_(nrtrcs)
    , logset_(&logset)
{
    TypeSet<MultiID> wllids;
    wllids += wllid;
    init( lognms, wllids );
}


LogCubeCreator::~LogCubeCreator()
{
    deepErase( welldata_ );
    deepErase( logcubes_ );
}


bool LogCubeCreator::init( const BufferStringSet& lognms,
			   const TypeSet<MultiID>& wllids )
{
    for ( int ilog=0; ilog<lognms.size(); ilog++ )
    {
	if ( !lognms.validIdx(ilog) )
	    continue;

	const BufferString& lognm = lognms.get( ilog );
	if ( lognm.isEmpty() )
	{
	    const uiString msg = tr("Log %1 is unnamed and will be skipped")
								.arg(ilog+1);
	    mErrRet(msg, errmsg_.isEmpty(), continue);
	}

	logcubes_ += new LogCube( lognm );
    }

    TypeSet<int> goodwells;
    for ( int iwell=0; iwell<wllids.size(); iwell++ )
    {
	const MultiID& wllid = wllids[iwell];
	WellData* welldata = new WellData( wllid );
	if ( !welldata->isOK() )
	{
	    BufferString wllnm;
	    if ( welldata->wd_ )
		wllnm = welldata->wd_->name();

	    const uiString msg = tr( "%1 for well %2" )
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
    mCheckInit( tr( "No wells to process" ) )

    errmsg_.setEmpty();
    uiString msg;
    if ( withwllnm && welldata_.size() > 1 )
    {
	msg = tr( "Cannot append well name with multiple wells" );
	mErrRet( msg, true, return false )
    }

    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.deftransl_ = "3D";
    ctxt.forread_ = false;
    ctxt.deftransl_ = CBVSSeisTrcTranslator::translKey();

    BufferString wellnmsuffix;
    if ( withwllnm )
    {
	if ( !welldata_.validIdx(0) || !welldata_[0]->wd_ )
	{
	    msg = tr( "Internal: no well name found" );
	    mErrRet( msg, true, return false )
	}

	wellnmsuffix.set( "from well " ).add( welldata_[0]->wd_->name() );
    }

    for ( int ilog=0; ilog<logcubes_.size(); ilog++ )
    {
	BufferString& fnm = logcubes_[ilog]->fnm_;
	if ( suffix )
	    fnm.addSpace().add( suffix );

	if ( withwllnm )
	    fnm.addSpace().add( wellnmsuffix );

	const IOObj* presentobj = IOM().getLocal( fnm.buf(),
						  ctxt.trgroup_->groupName() );
	if ( !presentobj )
	    continue;

	msg = tr( "Volume: '%1' is already present as another type"
		  " and won't be created" ).arg( fnm );
	if ( ctxt.deftransl_ != presentobj->translator() )
	    mErrRet( msg, errmsg_.isEmpty(), continue )

	msg = tr( "Volume: '%1' is already present" ).arg( fnm );
	mErrRet( msg, errmsg_.isEmpty(), continue )
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
    extractparams_.zstep_ = SI().zRange( true ).step;
    extractparams_.extractzintime_ = SI().zIsTime();
    extractparams_.snapZRangeToSurvey( true );

    mCheckInit( tr( "No wells to process" ) )
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
    errmsg_.append( errmsg, true ); \
    act; \
}

bool LogCubeCreator::doFinish( bool success )
{
    const uiString hdrmsg= tr("One or several log cubes could not be computed");
    uiString errmsg;

    if ( !nrDone() || errmsg_.isSet() )
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
	    const uiString msg = tr( "No data for log %1: no cube created" )
				     .arg( logcube.lognm_ );
	    mErrRet( msg, errmsg_.isEmpty(), continue )
	}

	if ( !logcube.makeWriteReady() )
	    mErrRet( logcube.errMsg(), errmsg_.isEmpty(), continue )

	trcsbufsout.sortForWrite( false );
	if ( !logcube.doWrite(trcsbufsout) )
	    mErrRet( logcube.errMsg(), errmsg_.isEmpty(), ; )
    }

    trcsbufsout.erase();

    if ( errmsg.isSet() )
	mAppendHdr( hdrmsg, return false )

    return true;
}


bool LogCubeCreator::makeLogTraces( int iwll )
{
    uiString msg;

    if ( !welldata_.validIdx(iwll) || !welldata_[iwll]->wd_ )
    {
	msg = tr( "Internal: No well data at iteration %1" )
		  .arg( toString(iwll) );
	mErrRet( msg, errmsg_.isEmpty(), return false )
    }

    ConstRefMan<Well::Data> wd = welldata_[iwll]->wd_;
    const BufferString wllnm = wd->name();
    if ( logcubes_.isEmpty() )
    {
	msg = tr( "Internal: No log cube data for well %1" ).arg( wllnm );
	mErrRet( msg, errmsg_.isEmpty(), return false )
    }

    if ( SI().zIsTime() && !wd->haveD2TModel() )
    {
	msg = tr("No depth/time model found for well %1").arg( wllnm );
	mErrRet( msg, errmsg_.isEmpty(), return false )
    }

    BufferStringSet lognms;
    getLogNames( lognms );
    PtrMan<Well::LogSampler> logsamp;
    if ( logset_ )
	logsamp = new Well::LogSampler( *wd, extractparams_, *logset_, lognms );
    else
	logsamp = new Well::LogSampler( *wd, extractparams_, lognms );

    if ( !logsamp->execute() )
    {
	msg = tr( "%1 for well %2" ).arg( logsamp->errMsg() ).arg( wllnm );
	mErrRet( msg, errmsg_.isEmpty(), return false )
    }

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
	{
	    msg = tr( "Cannot allocate memory for output log for well %1" )
		      .arg( wllnm );
	    mErrRet( msg, errmsg_.isEmpty(), return false )
	}

	logtrcs[ilog]->info().sampling = sampling;
	welldata_[iwll]->trcs_ += new SeisTrcBuf( true );
	logispresent += logset_ ? logset_->getLog( lognms.get(ilog).buf() )
				: wd->logs().getLog( lognms.get(ilog).buf() );
    }

    StepInterval<float> logzrg( logsamp->zRange().start, logsamp->zRange().stop,
				extractparams_.zstep_ );
    const int ns = logsamp->nrZSamples();
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
		logvals[ilog] = logsamp->getLogVal( ilog, idz );
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

	TrcKeySampling hrg(false);
	hrg.start_ = trackpos - bidvar;
	hrg.stop_ = trackpos + bidvar;
	hrg.snapToSurvey();

	TrcKeySamplingIterator hsit( hrg );
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
			trcsbufout.last()->info().setPos( bid );
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
    for ( int ilc=0; ilc<logcubes_.size(); ilc++ )
	lognms.add( logcubes_[ilc]->lognm_ );
}


void LogCubeCreator::addUniqueTrace( const SeisTrc& trc,
				     SeisTrcBuf& trcs ) const
{
    const BinID bid = trc.info().binID();
    const int pos = trcs.find( bid );
    if ( pos < 0 )
    {
	trcs.add( new SeisTrc(trc) );
	return;
    }

    SeisTrcPropChg stckr( *trcs.get( pos ) );
    stckr.stack( trc );
}
