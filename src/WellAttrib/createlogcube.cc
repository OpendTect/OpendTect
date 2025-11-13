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
	    mErrRet( writer.errMsg(), true, return false )
	}
    }

    delete seisioobj_;
    return true;
}


// LogCubeCreator::WellData

LogCubeCreator::WellData::WellData( const MultiID& wid,
				    const BufferStringSet* lognms )
{
    Well::LoadReqs lreqs( Well::Trck, Well::D2T );
    if ( lognms )
	lreqs.addLogs( *lognms );

    wd_ = Well::MGR().get( wid, lreqs );
    if ( !wd_ )
	mErrRet( Well::MGR().errMsg(), true, return )

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
    : ParallelTask("Log Cube creator")
    , extractparams_(pars)
    , stepout_(nrtrcs)
{
    TypeSet<MultiID> wllids;
    wllids += wllid;
    init( lognms, wllids );
}


LogCubeCreator::LogCubeCreator( const BufferStringSet& lognms,
				const TypeSet<MultiID>& wllids,
				const Well::ExtractParams& pars, int nrtrcs )
    : ParallelTask("Log Cube creator")
    , extractparams_(pars)
    , stepout_(nrtrcs)
{
    init( lognms, wllids );
}


LogCubeCreator::LogCubeCreator(const BufferStringSet& lognms,
			       const Well::LogSet& logset, const MultiID& wllid,
			       const Well::ExtractParams& pars, int nrtrcs )
    : ParallelTask("Log Cube creator")
    , logset_(&logset)
    , extractparams_(pars)
    , stepout_(nrtrcs)
{
    TypeSet<MultiID> wllids;
    wllids += wllid;
    init( lognms, wllids );
}


LogCubeCreator::~LogCubeCreator()
{
    deepErase( logcubes_ );
    deepErase( welldata_ );
}


bool LogCubeCreator::isOK() const
{
    return !welldata_.isEmpty() && uirv_.isOK();
}


uiString LogCubeCreator::uiNrDoneText() const
{
    return tr("Wells handled");
}


uiRetVal LogCubeCreator::allMessages() const
{
    uiRetVal uirv( msg_ );
    uirv.add( uirv_ );
    return uirv;
}


void LogCubeCreator::init( const BufferStringSet& lognms,
			   const TypeSet<MultiID>& wellids )
{
    msg_ = tr("Extracting well data");
    deepErase( logcubes_ );
    for ( const auto* lognm : lognms )
	logcubes_.add( new LogCube(lognm->buf()) );

    deepErase( welldata_ );
    for ( const auto& wid : wellids )
    {
	PtrMan<WellData> welldata =
			new WellData( wid, logset_ ? nullptr : &lognms );
	if ( !welldata->isOK() )
	{
	    BufferString wllnm;
	    if ( welldata->wd_ )
		wllnm = welldata->wd_->name();

	    uirv_.add( tr( "%1 for well %2" )
			.arg( welldata->errMsg() ).arg( wllnm ) );
	    continue;
	}

	welldata_.add( welldata.release() );
    }
}


uiRetVal LogCubeCreator::setOutputNm( const char* suffix, bool withwllnm,
				      uiStringSet& existimpls )
{
    if ( !isOK() )
    {
	uiRetVal uirv = tr("No valid wells to process");
	uirv.add( details() );
	return uirv;
    }

    uiString msg;
    if ( withwllnm && welldata_.size() > 1 )
	return tr( "Cannot append well name with multiple wells" );

    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.deftransl_ = "3D";
    ctxt.forread_ = false;
    ctxt.deftransl_ = CBVSSeisTrcTranslator::translKey();

    BufferString wellnmsuffix;
    if ( withwllnm )
    {
	if ( welldata_.isEmpty() || !welldata_.first()->wd_ )
	    return tr( "Internal: no well name found" );

	wellnmsuffix.set( "from well " ).add( welldata_.first()->wd_->name() );
    }

    uiRetVal uirv;
    for ( auto* logcube : logcubes_ )
    {
	BufferString& fnm = logcube->fnm_;
	if ( suffix )
	    fnm.addSpace().add( suffix );

	if ( withwllnm )
	    fnm.addSpace().add( wellnmsuffix );

	const IOObj* presentobj = IOM().get( fnm.buf(),
					     ctxt.trgroup_->groupName() );
	if ( !presentobj )
	    continue;

	msg = tr( "Volume: '%1' is already present as another type"
		  " and won't be created" ).arg( fnm );
	if ( ctxt.deftransl_ != presentobj->translator() )
	{
	    uirv.add( tr( "Volume: '%1' is already present as another type"
			" and won't be created" ).arg( fnm ) );
	    continue;
	}

	existimpls.add( tr( "Volume: '%1' is already present" ).arg( fnm ) );
    }

    return uirv;
}


void LogCubeCreator::getOutputNames( BufferStringSet& names ) const
{
    for ( const auto* logcube : logcubes_ )
	names.add( logcube->fnm_.buf() );
}


bool LogCubeCreator::doPrepare( int /* nrthreads */ )
{
    if ( !isOK() )
	return false;

    for ( const auto* logcube : logcubes_ )
    {
	if ( logcube->fnm_.isEmpty() )
	{
	    msg_ = tr("No output name has been set");
	    return false;
	}
    }

    extractparams_.zstep_ = SI().zRange( true ).step_;
    extractparams_.extractzintime_ = SI().zIsTime();
    extractparams_.snapZRangeToSurvey( true );

    msg_ = tr("Extracting well data");
    uirv_.setOK();

    return true;
}


bool LogCubeCreator::doWork( od_int64 start, od_int64 stop,
			     int /* threadidx */ )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	const uiString msg = makeLogTraces(idx);
	if ( msg.isSet() )
	    uirv_.add( msg );
    }

    return true;
}


bool LogCubeCreator::doFinish( bool success )
{
    const uiString hdrmsg= tr("One or several log cubes could not be computed");
    if ( !success || !isOK() )
    {
	msg_ = hdrmsg;
	return false;
    }

    uiRetVal uirv;
    for ( int ilog=0; ilog<logcubes_.size(); ilog++ )
    {
	SeisTrcBuf trcsbufsout( true );
	for ( const auto* welldata : welldata_ )
	{
	    if ( !welldata->trcs_.validIdx(ilog) )
		continue;

	    const SeisTrcBuf& welltrcs = *welldata->trcs_.get( ilog );
	    for ( int itrc=0; itrc<welltrcs.size(); itrc++ )
		addUniqueTrace( *welltrcs.get(itrc), trcsbufsout );
	}

	LogCube& logcube = *logcubes_[ilog];
	if ( trcsbufsout.isEmpty() )
	{
	    uirv.add( tr( "No data for log %1: no cube created" )
			.arg( logcube.lognm_ ) );
	    continue;
	}

	if ( !logcube.makeWriteReady() )
	{
	    uirv.add( logcube.errMsg() );
	    continue;
	}

	trcsbufsout.sortForWrite( false );
	if ( !logcube.doWrite(trcsbufsout) )
	{
	    uirv.add( logcube.errMsg() );
	    continue;
	}
    }

    if ( uirv.isError() )
    {
	msg_ = hdrmsg;
	uirv_ = uirv;
	return false;
    }

    return true;
}


uiString LogCubeCreator::makeLogTraces( int iwll )
{
    uiString msg;
    if ( !welldata_.validIdx(iwll) || !welldata_[iwll]->wd_ )
    {
	return tr( "Internal: No well data at iteration %1" )
		  .arg( toString(iwll) );
    }

    ConstRefMan<Well::Data> wd = welldata_[iwll]->wd_;
    const char* wllnm = wd->name().buf();
    if ( !logcubes_.validIdx(iwll) )
	return tr( "Internal: No log cube data for well %1" ).arg( wllnm );

    if ( SI().zIsTime() && !wd->haveD2TModel() )
	return tr("No depth/time model found for well %1").arg( wllnm );

    BufferStringSet lognms;
    getLogNames( lognms );
    PtrMan<Well::LogSampler> logsamp;
    if ( logset_ )
	logsamp = new Well::LogSampler( *wd, extractparams_, *logset_, lognms );
    else
	logsamp = new Well::LogSampler( *wd, extractparams_, lognms );

    if ( !logsamp->execute() )
	return tr( "%1 for well %2" ).arg( logsamp->errMsg() ).arg( wllnm );

    const SamplingData<float> sampling( SI().zRange( true ) );
    const int trcsz = SI().zRange( true ).nrSteps() + 1;
    SeisTrc undeftrc( trcsz );
    undeftrc.info().sampling_ = sampling;
    undeftrc.setAll( mUdf(float) );

    ObjectSet<SeisTrc> logtrcs;
    BoolTypeSet logispresent;
    for ( int ilog=0; ilog<lognms.size(); ilog++ )
    {
	logtrcs += new SeisTrc( undeftrc );
	if ( !logtrcs.validIdx(ilog) )
	    return tr("Cannot allocate memory for output log for well %1")
		      .arg( wllnm );

	logtrcs[ilog]->info().sampling_ = sampling;
	welldata_[iwll]->trcs_ += new SeisTrcBuf( true );
	logispresent += logset_ ? logset_->getLog( lognms.get(ilog).buf() )
				: wd->logs().getLog( lognms.get(ilog).buf() );
    }

    const ZSampling logzrg( logsamp->zRange().start_, logsamp->zRange().stop_,
			    extractparams_.zstep_ );
    const int ns = logsamp->nrZSamples();
    const int nrlogs = logcubes_.size();
    TypeSet<float> logvals;
    logvals.setSize( nrlogs );
    const int icomp = 0;
    for ( int idztrc=0; idztrc<trcsz; idztrc++ )
    {
	const float depth = sampling.atIndex( idztrc );
	logvals.setAll( mUdf(float) );
	if ( logzrg.includes(depth,true) )
	{
	    const int idz = logzrg.getIndex( depth );
	    if ( idz<0 || idz>=ns )
		continue;

	    for ( int ilog=0; ilog<lognms.size(); ilog++ )
		logvals[ilog] = logsamp->getLogVal( ilog, idz );
	}

	for ( int ilog=0; ilog<lognms.size(); ilog++ )
	{
	    if ( logispresent[ilog] )
		logtrcs[ilog]->set( idztrc, logvals[ilog], icomp );
	}
    }

    logsamp = nullptr;
    const BinID bidvar( stepout_, stepout_);
    const od_int64 sqstep = stepout_* stepout_;
    const TypeSet<BinID>& trackbinids = welldata_[iwll]->binidsalongtrack_;
    for ( int itrckpt=0; itrckpt<trackbinids.size(); itrckpt++ )
    {
	const BinID& trackpos = trackbinids[itrckpt];
	for ( int ilog=0; ilog<nrlogs; ilog++ )
	{
	    logvals[ilog] = logtrcs.validIdx(ilog) && logispresent[ilog] ?
			    logtrcs[ilog]->get( itrckpt, icomp ) : mUdf(float);
	}

	TrcKeySampling tks( Survey::default3DGeomID() );
	tks.start_ = trackpos - bidvar;
	tks.stop_ = trackpos + bidvar;
	tks.snapToSurvey();

	TrcKeySamplingIterator hsit( tks );
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
			auto* newtrc = new SeisTrc( undeftrc );
			newtrc->info().setPos( bid );
			trcsbufout.add( newtrc );
			trcpos = trcsbufout.size() - 1;
		    }

		    const float logval = logvals[ilog];
		    if ( mIsUdf(logval) )
			continue;

		    trcsbufout.get( trcpos )->set( itrckpt, logval, icomp );
		}
	    }
	}
    }

    deepErase( logtrcs );

    return uiString::empty();
}


void LogCubeCreator::getLogNames( BufferStringSet& lognms ) const
{
    for ( const auto* logcube : logcubes_ )
	lognms.add( logcube->lognm_.buf() );
}


void LogCubeCreator::addUniqueTrace( const SeisTrc& trc, SeisTrcBuf& trcs )
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
