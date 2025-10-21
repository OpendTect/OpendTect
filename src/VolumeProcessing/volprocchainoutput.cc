/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volprocchainoutput.h"

#include "volprocchainexec.h"
#include "volproctrans.h"
#include "seisdatapackwriter.h"
#include "ioman.h"
#include "jobcommunic.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "veldesc.h"
#include "threadwork.h"
#include "progressmeterimpl.h"


VolProc::ChainOutput::ChainOutput()
    : Executor("Volume Processing Output")
    , cs_(true)
    , progresskeeper_(*new ProgressRecorder)
    , tkscalcscope_(cs_.hsamp_)
{
    setProgressMeter( &progresskeeper_ );
}


VolProc::ChainOutput::~ChainOutput()
{
    delete wrr_;
    deepErase( storers_ );
    chain_ = nullptr;
    delete chainexec_;
    delete chainpar_;
    delete &progresskeeper_;
}


void VolProc::ChainOutput::setChainID( const MultiID& chainid )
{
    chainid_ = chainid;
}


void VolProc::ChainOutput::setOutputID( const MultiID& outid )
{
    outid_ = outid;
}


void VolProc::ChainOutput::setTrcKeyZSampling( const TrcKeyZSampling& tkzs )
{
    cs_ = tkzs;
}


void VolProc::ChainOutput::usePar( const IOPar& iop )
{
    iop.get( VolProcessingTranslatorGroup::sKeyChainID(), chainid_ );
    chain_ = nullptr;
    if ( chainid_.isUdf() || chainid_.isUdf() )
    {
	if ( chainpar_ )
	    deleteAndNullPtr( chainpar_ );

	chainpar_ = iop.subselect( sKey::Chain() );
	if ( !chainpar_ )
	    return;
    }

    PtrMan<IOPar> subselpar = iop.subselect(
	    IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( subselpar )
       cs_.usePar( *subselpar );

    iop.get( "Output.0.Seismic.ID", outid_ );

    const bool repsimple = ReportingTask::needSimpleLogging( iop );
    int repperc = 5;
    if ( repsimple )
	iop.get( ReportingTask::sKeySimpleLoggingStep(), repperc );

    setSimpleMeter( repsimple, repperc );
}


void VolProc::ChainOutput::setProgressMeter( ProgressMeter* pm )
{
    progresskeeper_.setForwardTo( pm );
    progresskeeper_.skipProgress( true );
}


void VolProc::ChainOutput::setJobCommunicator( JobCommunic* jc )
{
    jobcomm_ = jc;
}


void VolProc::ChainOutput::enableWorkControl( bool yn )
{
    if ( chainexec_ )
	chainexec_->enableWorkControl( workControlEnabled() );
    if ( wrr_ )
	wrr_->enableWorkControl( workControlEnabled() );

    Executor::enableWorkControl( yn );
}


void VolProc::ChainOutput::controlWork( Control ctrl )
{
    if ( !workControlEnabled() )
	return;

    if ( chainexec_ )
	chainexec_->controlWork( ctrl );
    if ( wrr_ )
	wrr_->controlWork( ctrl );

    Executor::controlWork( ctrl );
}


void VolProc::ChainOutput::createNewChainExec()
{
    deleteAndNullPtr( chainexec_ );
    chain_ = nullptr;
    if ( getChain() != MoreToDo() )
    {
	chain_ = nullptr;
	return;
    }
    /* Many usePar implementations also allocate auxiliary data:
       Restore in case of chunking */

    chainexec_ = new VolProc::ChainExecutor( *chain_ );
    chainexec_->enableWorkControl( workControlEnabled() );
    ((Task*)chainexec_)->setProgressMeter( progresskeeper_.forwardTo() );
    chainexec_->setSimpleMeter( useSimpleMeter(), simpleMeterStep() );
    chainexec_->setJobCommunicator( jobcomm_ );
}


int VolProc::ChainOutput::retError( const uiString& msg )
{
    progresskeeper_.setMessage( msg );
    setName( "Volume builder processing" );

    return ErrorOccurred();
}


int VolProc::ChainOutput::retMoreToDo()
{ return MoreToDo(); }


od_int64 VolProc::ChainOutput::nrDone() const
{
    return progresskeeper_.nrDone();
}


od_int64 VolProc::ChainOutput::totalNr() const
{
    return chainexec_ ? progresskeeper_.totalNr() : 1;
}


uiString VolProc::ChainOutput::uiNrDoneText() const
{
    return progresskeeper_.nrDoneText();
}


uiString VolProc::ChainOutput::uiMessage() const
{
    return progresskeeper_.message();
}


int VolProc::ChainOutput::nextStep()
{
    if ( !shouldContinue() )
	return Finished();

    if ( nrexecs_<0 )
    {
	tkscalcscope_ = cs_.hsamp_;
	tkscalcdone_.init(false);
	return setupChunking();
    }
    else if ( neednextchunk_ )
	return setNextChunk();

    Threads::Locker slock( storerlock_ );
    manageStorers();
    if ( storererr_ )
	return ErrorOccurred();
    slock.unlockNow();

    if ( chainexec_ )
    {
	int res = chainexec_->doStep();
	if ( res < 0 )
	    return retError( chainexec_->errMsg() );

	if ( res == 0 )
	{
	    if ( !wrr_ && !openOutput() )
		return ErrorOccurred();

	    neednextchunk_ = ++curexecnr_ < nrexecs_;
	    startWriteChunk();
	    if ( !neednextchunk_ )
	    {	//We just did the last step of the last chunk
		setProgressMeter( nullptr );
		deleteAndNullPtr( chainexec_ );
	    }
	}

	return MoreToDo();
    }

    slock.reLock();
    if ( !storers_.isEmpty() )
    {
	slock.unlockNow();
	Threads::sleep( 0.1 );
	return MoreToDo();
    }

    // no calculations going on, no storers left ...
    setProgressMeter( &progresskeeper_ );
    setName( "Volume builder processing" );

    return Finished();
}


int VolProc::ChainOutput::getChain()
{
    if ( chainpar_ )
    {
	chain_ = new Chain;
	return chain_->usePar( *chainpar_ ) ? MoreToDo()
					    : retError( chain_->errMsg() );
    }

    if ( chainid_.isUdf() )
	return retError( tr("No Volume Processing ID specified") );

    PtrMan<IOObj> ioobj = IOM().get( chainid_ );
    if ( !ioobj )
	return retError( uiStrings::phrCannotFind(
		tr("Volume Processing with id: %1").arg(chainid_) ) );

    chain_ = new Chain;
    uiString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj.ptr(),errmsg) )
	return retError( errmsg );
    else if ( chain_->nrSteps() < 1 )
	return retError( tr("Empty Volume Processing Chain - nothing to do.") );

    const Step& step0 = *chain_->getStep(0);
    if ( step0.needsInput() )
    {
	return retError(
		tr("First step in Volume Processing Chain (%1) requires input."
		    "\nIt can thus not be first.").arg( step0.userName() ) );
    }

    chainpar_ = new IOPar;
    chain_->fillPar( *chainpar_ );
    if ( chainpar_->isEmpty() )
	deleteAndNullPtr( chainpar_ );

    return MoreToDo();
}


int VolProc::ChainOutput::setupChunking()
{
    createNewChainExec();
    if ( !chainexec_ )
	return ErrorOccurred();

    chainexec_->chain_.setZStep( cs_.zsamp_.step_, SI().zIsTime() );
    /* chain_.zstep_ is not used, but setting it for external plugin builders
       in case they read chain_.getZStep() */

    const TrcKeySampling& tks = tkscalcscope_;
    outputzrg_ = StepInterval<int>( 0, cs_.zsamp_.nrSteps(), 1 );
    od_uint64 memusage;
    uiString errmsg;
    if ( !chainexec_->setCalculationScope(tks,cs_.zsamp_,memusage,&nrexecs_) )
	return retError( tr("Processing aborted: %1")
			.arg( chainexec_->errMsg() ) );

    neednextchunk_ = true;
    curexecnr_ = 0;
    return MoreToDo();
}


int VolProc::ChainOutput::setNextChunk()
{
    neednextchunk_ = false;
    if ( curexecnr_ > 0 )
    {
	createNewChainExec();
	if ( !chainexec_ )
	    return ErrorOccurred();
    }

    TrcKeySampling& scopetks = tkscalcscope_;
    const TrcKeySampling hsamp( scopetks.getLineChunk(nrexecs_,curexecnr_) );
    od_uint64 memusage; int nrsubexecs;
    if ( !chainexec_->setCalculationScope(hsamp,cs_.zsamp_,memusage,
					  &nrsubexecs) )
	return retError( chainexec_->errMsg() );
    else if ( nrsubexecs > 1 )
    {
	scopetks.start_.lineNr() = hsamp.start_.lineNr();
	return setupChunking();
    }
    else
    {
	TrcKeySampling& calcdonetks = tkscalcdone_;
	if ( calcdonetks.isEmpty() )
	    calcdonetks = hsamp;
	else
	    calcdonetks.stop_.lineNr() = hsamp.stop_.lineNr();
    }

    if ( nrexecs_ < 2 )
	return MoreToDo();

    progresskeeper_.setMessage(
	    tr("Starting new Volume Processing chunk %1-%2.")
	    .arg( hsamp.start_.inl() ).arg( hsamp.stop_.inl() ), true );
    progresskeeper_.setMessage( uiString::empty() );

    return MoreToDo();
}


#define mErrRet( msg ) { retError( msg ); return false; }

bool VolProc::ChainOutput::openOutput()
{
    ConstRefMan<RegularSeisDataPack> seisdp = chainexec_->getOutput();
    if ( !seisdp )
	mErrRet( tr("No output data available") )

    PtrMan<IOObj> ioobj = IOM().get( outid_ );
    if ( !ioobj )
	mErrRet( uiStrings::phrCannotFind( tr("output cube ID in database") ) )

    const VelocityDesc* vd = chain_->getVelDesc();
    ConstPtrMan<VelocityDesc> veldesc = vd ? new VelocityDesc( *vd ) : 0;
    bool docommit = false;
    VelocityDesc omfdesc;
    const bool hasveldesc = omfdesc.usePar( ioobj->pars() );
    if ( veldesc )
    {
	if ( !hasveldesc || omfdesc != *veldesc )
	    { veldesc->fillPar( ioobj->pars() ); docommit = true; }
    }
    else if ( hasveldesc )
	{ VelocityDesc::removePars( ioobj->pars() ); docommit = true; }
    if ( docommit )
	IOM().commitChanges( *ioobj );

    delete wrr_;
    wrr_ = new SeisDataPackWriter( outid_, *seisdp );
    seisdp = nullptr;

    wrr_->setSelection( cs_.hsamp_, outputzrg_ );
    for ( int idx=0; idx<chain_->getOutputScalers().size(); idx++ )
    {
	const Scaler* scaler = chain_->getOutputScalers()[idx];
	if ( !scaler )
	    continue;

	wrr_->setComponentScaler( *scaler, idx );
    }

    wrr_->enableWorkControl( workControlEnabled() );

    return true;
}


namespace VolProc
{

class ChainOutputStorer : public CallBacker
{ mODTextTranslationClass(ChainOutputStorer)
public:

ChainOutputStorer( ChainOutput& co, const RegularSeisDataPack& dp )
    : co_(co)
    , dp_(&dp)
{
}

~ChainOutputStorer()
{
    if ( work_ )
	Threads::WorkManager::twm().removeWork( *work_ );
}


bool hasWork() const
{ return work_; }


void startWork()
{
    if ( !dp_ )
	return;

    SeisDataPackWriter& wrr = *co_.wrr_;
    if ( wrr.dataPack() == dp_.ptr() )
	wrr.setSelection( dp_->sampling().hsamp_, wrr.zSampling() );
    else
	wrr.setNextDataPack( *dp_ );

    dp_ = nullptr;

    //Disabling silent background writing:
/*    if ( co_.nrexecs_ == co_.curexecnr_ )
	wrr.setProgressMeter( co_.progresskeeper_.forwardTo() );

    work_ = new Threads::Work( wrr, false );
    CallBack finishedcb( mCB(this,VolProc::ChainOutputStorer,workFinished) );
    Threads::WorkManager::twm().addWork( *work_, &finishedcb );*/

    ((Task&)wrr).setProgressMeter( co_.progresskeeper_.forwardTo() );
    wrr.setSimpleMeter( co_.useSimpleMeter(), co_.simpleMeterStep() );
    wrr.execute();
    co_.reportFinished( *this );
}


void workFinished( CallBacker* cb )
{
    const bool isfail = !Threads::WorkManager::twm().getWorkExitStatus( cb );
    if ( isfail )
    {
	errmsg_ = co_.wrr_->uiMessage();
	if ( errmsg_.isEmpty() )
	    errmsg_ = tr("Error during background write");
    }
    deleteAndNullPtr( work_ );
    co_.reportFinished( *this );
}

    uiString		    errmsg_;

private:

    ChainOutput&	    co_;
    ConstRefMan<RegularSeisDataPack> dp_;
    Threads::Work*	    work_	= nullptr;

};

} // namespace VolProc


void VolProc::ChainOutput::startWriteChunk()
{
    ConstRefMan<RegularSeisDataPack> dp = chainexec_->getOutput();
    if ( !dp )
	return;

    Threads::Locker slock( storerlock_ );
    storers_ += new ChainOutputStorer( *this, *dp );
    dp = nullptr;

    //Disabling silent background writing:
    manageStorers(); //Starts writing
    manageStorers(); //Deletes finished storer
}


void VolProc::ChainOutput::reportFinished( ChainOutputStorer& storer )
{
    Threads::Locker slock( storerlock_ );

    toremstorers_ += &storer;
    storers_ -= &storer;
    if ( !storer.errmsg_.isEmpty() )
    {
	progresskeeper_.setMessage( storer.errmsg_ );
	storererr_ = true;
    }
}


void VolProc::ChainOutput::manageStorers()
{
	// already locked when called

    while ( !toremstorers_.isEmpty() )
	delete toremstorers_.removeSingle( 0 );

    if ( storererr_ )
	{ deepErase( storers_ ); return; }

    if ( storers_.isEmpty() || storers_[0]->hasWork() )
	return;

    storers_[0]->startWork();
}
