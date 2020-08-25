/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    , chainid_(MultiID::udf())
    , chainpar_(0)
    , chain_(0)
    , chainexec_(0)
    , wrr_(0)
    , neednextchunk_(true)
    , nrexecs_(-1)
    , curexecnr_(-1)
    , storererr_(false)
    , progresskeeper_(*new ProgressRecorder)
    , jobcomm_(0)
    , tkscalcscope_(cs_.hsamp_)
{
    progressmeter_ = &progresskeeper_;
}


VolProc::ChainOutput::~ChainOutput()
{
    delete wrr_;
    deepErase( storers_ );

    if ( chain_ )
	chain_->unRef();

    delete chainexec_;
    delete chainpar_;
    delete &progresskeeper_;
}


void VolProc::ChainOutput::setChainID( const MultiID& chainid )
{ chainid_ = chainid; }

void VolProc::ChainOutput::setOutputID( const MultiID& outid )
{ outid_ = outid; }

void VolProc::ChainOutput::setTrcKeyZSampling( const TrcKeyZSampling& tkzs )
{ cs_ = tkzs; }

void VolProc::ChainOutput::usePar( const IOPar& iop )
{
    iop.get( VolProcessingTranslatorGroup::sKeyChainID(), chainid_ );
    unRefAndZeroPtr( chain_ );
    if ( chainid_.isEmpty() || chainid_.isUdf() )
    {
	if ( chainpar_ )
	    deleteAndZeroPtr( chainpar_ );

	chainpar_ = iop.subselect( sKey::Chain() );
	if ( !chainpar_ ) return;
    }

    PtrMan<IOPar> subselpar = iop.subselect(
	    IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( subselpar )
       cs_.usePar( *subselpar );

    iop.get( "Output.0.Seismic.ID", outid_ );
}


void VolProc::ChainOutput::setProgressMeter( ProgressMeter* pm )
{
    progresskeeper_.setForwardTo( pm );
    progresskeeper_.skipProgress( true );
}


void VolProc::ChainOutput::setJobCommunicator( JobCommunic* jc )
{ jobcomm_ = jc; }


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
    deleteAndZeroPtr( chainexec_ );
    if ( chain_ )
	unRefAndZeroPtr( chain_ );

    if ( getChain() != MoreToDo() )
    {
	unRefAndZeroPtr( chain_ );
	return;
    }
    /* Many usePar implementations also allocate auxiliary data:
       Restore in case of chunking */

    chainexec_ = new VolProc::ChainExecutor( *chain_ );
    chainexec_->enableWorkControl( workControlEnabled() );
    chainexec_->setProgressMeter( progresskeeper_.forwardTo() );
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
    return progresskeeper_.totalNr();
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
		progressmeter_ = 0;
		deleteAndZeroPtr( chainexec_ );
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
    progressmeter_ = &progresskeeper_;
    setName( "Volume builder processing" );

    return Finished();
}


int VolProc::ChainOutput::getChain()
{
    if ( chainpar_ )
    {
	chain_ = new Chain; chain_->ref();
	return chain_->usePar( *chainpar_ ) ? MoreToDo()
					    : retError( chain_->errMsg() );
    }

    if ( chainid_.isEmpty() )
	return retError( tr("No Volume Processing ID specified") );

    PtrMan<IOObj> ioobj = IOM().get( chainid_ );
    if ( !ioobj )
	return retError( uiStrings::phrCannotFind(
		tr("Volume Processing with id: %1").arg(chainid_) ) );

    chain_ = new Chain; chain_->ref();
    uiString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj,errmsg) )
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
	deleteAndZeroPtr( chainpar_ );

    return MoreToDo();
}


int VolProc::ChainOutput::setupChunking()
{
    createNewChainExec();
    if ( !chainexec_ )
	return ErrorOccurred();

    chainexec_->chain_.setZStep( cs_.zsamp_.step, SI().zIsTime() );
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
    progresskeeper_.setMessage( uiString::emptyString() );

    return MoreToDo();
}


#define mErrRet( msg ) { retError( msg ); return false; }

bool VolProc::ChainOutput::openOutput()
{
    const RegularSeisDataPack* seisdp = chainexec_->getOutput();
    if ( !seisdp || !DPM( DataPackMgr::SeisID() ).obtain(seisdp->id()) )
    {
	const_cast<RegularSeisDataPack*>( seisdp )->release();
	mErrRet( tr("No output data available") )
    }

    PtrMan<IOObj> ioobj = IOM().get( outid_ );
    if ( !ioobj )
    {
	DPM( DataPackMgr::SeisID() ).release( seisdp->id() );
	mErrRet( uiStrings::phrCannotFind( tr("output cube ID in database") ) )
    }

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
    DPM( DataPackMgr::SeisID() ).release( seisdp->id() );
    seisdp = 0;

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
    , work_(0)
{
    if ( !DPM( DataPackMgr::SeisID() ).obtain(dp.id()) )
    {
	const_cast<RegularSeisDataPack*>( dp_ )->release();
	errmsg_ = tr("Error during background write");
	dp_ = 0;
    }
}

~ChainOutputStorer()
{
    DPM( DataPackMgr::SeisID() ).release( dp_ );
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
    if ( wrr.dataPack() == dp_ )
	wrr.setSelection( dp_->sampling().hsamp_, wrr.zSampling() );
    else
	wrr.setNextDataPack( *dp_ );

    DPM( DataPackMgr::SeisID() ).release( dp_ );
    dp_ = 0;

    //Disabling silent background writing:
/*    if ( co_.nrexecs_ == co_.curexecnr_ )
	wrr.setProgressMeter( co_.progresskeeper_.forwardTo() );

    work_ = new Threads::Work( wrr, false );
    CallBack finishedcb( mCB(this,VolProc::ChainOutputStorer,workFinished) );
    Threads::WorkManager::twm().addWork( *work_, &finishedcb );*/

    wrr.setProgressMeter( co_.progresskeeper_.forwardTo() );
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
    deleteAndZeroPtr( work_ );
    co_.reportFinished( *this );
}

    uiString		    errmsg_;

private:

    ChainOutput&	    co_;
    const RegularSeisDataPack* dp_;
    Threads::Work*	    work_;

};

} // namespace VolProc


void VolProc::ChainOutput::startWriteChunk()
{
    const RegularSeisDataPack* dp = chainexec_->getOutput();
    if ( !dp )
	return;

    Threads::Locker slock( storerlock_ );
    storers_ += new ChainOutputStorer( *this, *dp );
    DPM( DataPackMgr::SeisID() ).release( dp );
    chainexec_->outputdp_ = 0; //The chain executor no longer needs it

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
    wrr_->releaseDP();
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
