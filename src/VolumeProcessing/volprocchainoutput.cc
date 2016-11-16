/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/


#include "volprocchainoutput.h"

#include "volprocchainexec.h"
#include "volproctrans.h"
#include "seisdatapackwriter.h"
#include "dbman.h"
#include "jobcommunic.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "veldesc.h"
#include "odsysmem.h"
#include "threadwork.h"
#include "progressmeterimpl.h"


VolProc::ChainOutput::ChainOutput()
    : Executor("")
    , tkzs_(true)
    , chain_(0)
    , chainexec_(0)
    , wrr_(0)
    , neednextchunk_(true)
    , nrexecs_(-1)
    , curexecnr_(-1)
    , storererr_(false)
    , progresskeeper_(*new ProgressRecorder)
    , comm_(0)
{
    progressmeter_ = &progresskeeper_;
}


VolProc::ChainOutput::~ChainOutput()
{
    chain_->unRef();
    delete chainexec_; chainexec_ = 0;
    delete wrr_;
    deepErase( storers_ );
    delete &progresskeeper_;
}


void VolProc::ChainOutput::setChainID( const DBKey& chainid )
{ chainid_ = chainid; }

void VolProc::ChainOutput::setOutputID( const DBKey& outid )
{ outid_ = outid; }

void VolProc::ChainOutput::setTrcKeyZSampling( const TrcKeyZSampling& tkzs )
{ tkzs_ = tkzs; }

void VolProc::ChainOutput::setJobComm(JobCommunic* comm)
{
    comm_ = comm;
    if ( comm_ )
	comm_->setProgressDetail( "percent done" );
}


void VolProc::ChainOutput::usePar( const IOPar& iop )
{
    iop.get( VolProcessingTranslatorGroup::sKeyChainID(), chainid_ );

    PtrMan<IOPar> subselpar = iop.subselect(
	    IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( subselpar )
       tkzs_.usePar( *subselpar );

    iop.get( "Output.0.Seismic.ID", outid_ );
}


void VolProc::ChainOutput::setProgressMeter( ProgressMeter* pm )
{
    progresskeeper_.setForwardTo( pm );
    progresskeeper_.skipProgress( true );
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
    delete chainexec_;
    chainexec_ = new VolProc::ChainExecutor( *chain_ );
    chainexec_->enableWorkControl( workControlEnabled() );
    chainexec_->setProgressMeter( progresskeeper_.forwardTo() );
}


int VolProc::ChainOutput::retError( const uiString& msg )
{
    progresskeeper_.setMessage( msg );
    return ErrorOccurred();
}


od_int64 VolProc::ChainOutput::nrDone() const
{
    return progresskeeper_.nrDone();
}


od_int64 VolProc::ChainOutput::totalNr() const
{
    return progresskeeper_.totalNr();
}


uiString VolProc::ChainOutput::nrDoneText() const
{
#ifdef __debug__
    uiString ret = progresskeeper_.nrDoneText();
    if ( FixedString(ret.getFullString())=="Nr Done" )
	ret = toUiString("# done");
    return ret;
#else
    return progresskeeper_.nrDoneText();
#endif
}


uiString VolProc::ChainOutput::message() const
{
    return progresskeeper_.message();
}


int VolProc::ChainOutput::nextStep()
{
    if ( !shouldContinue() )
	return Finished();

    if ( !chain_ )
	return getChain();
    else if ( nrexecs_<0 )
	return setupChunking();
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
	    return ErrorOccurred();

	if ( res == 0 )
	{
	    if ( !wrr_ && !openOutput() )
		return ErrorOccurred();

	    neednextchunk_ = ++curexecnr_ < nrexecs_;
	    startWriteChunk();
	}

	if ( comm_ )
	{
	    const double curperc = mCast(double,chainexec_->nrDone());
	    const int totperc = mNINT32((100*curexecnr_ + curperc) / nrexecs_ );
	    comm_->sendProgress( totperc );
	}

	if ( res == 0 && !neednextchunk_ )
	{   //We just did the last step of the last chunk
	    progressmeter_ = 0;
	    deleteAndZeroPtr( chainexec_ );
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
    if ( chainid_.isInvalid() )
	return retError( tr("No Volume Processing ID specified") );

    PtrMan<IOObj> ioobj = DBM().get( chainid_ );
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

    return MoreToDo();
}


int VolProc::ChainOutput::setupChunking()
{
    createNewChainExec();

    const float zstep = chain_->getZStep();
    outputzrg_ = StepInterval<int>( mNINT32(tkzs_.zsamp_.start/zstep),
			mNINT32(Math::Ceil(tkzs_.zsamp_.stop/zstep)),
			mMAX(mNINT32(Math::Ceil(tkzs_.zsamp_.step/zstep)),1) );
			   //real -> index, outputzrg_ is the index of z-samples

    // We will be writing while a new chunk will be calculated
    // Thus, we need to keep the output datapack alive while the next chunk
    // is calculated. Therefore, lets double the computed mem need:

    od_uint64 nrbytes = 2 * chainexec_->computeMaximumMemoryUsage( tkzs_.hsamp_,
								   outputzrg_ );
    od_int64 totmem, freemem; OD::getSystemMemory( totmem, freemem );

    /* handy for test:
	if ( freemem > nrbytes ) freemem = nrbytes - 100; // min 2 chunks
	if ( freemem > nrbytes/2 ) freemem = nrbytes/2 - 100; // min 3 chunks
    */

    bool needsplit = freemem >= 0 && nrbytes > freemem;
    const bool cansplit = chainexec_->areSamplesIndependent()
			&& !chainexec_->needsFullVolume();
    nrexecs_ = 1;
    if ( needsplit && !cansplit )
    {
	// duh. but ... it may still fit, fingers crossed:
	nrbytes /= 2;
	needsplit = nrbytes > freemem;
	if ( needsplit )
	    return retError(
		    tr("Processing aborted; not enough available memory.") );
    }
    if ( needsplit )
    {
	nrexecs_ = (int)(nrbytes / freemem) + 1;
	if ( nrexecs_ > tkzs_.hsamp_.nrLines() )
	    nrexecs_ = tkzs_.hsamp_.nrLines(); // and pray!
    }

    neednextchunk_ = true;
    curexecnr_ = 0;
    return MoreToDo();
}


int VolProc::ChainOutput::setNextChunk()
{
    neednextchunk_ = false;
    if ( curexecnr_ > 0 )
	createNewChainExec();

    const TrcKeySampling hsamp( tkzs_.hsamp_.getLineChunk(nrexecs_,curexecnr_));

    if ( !chainexec_->setCalculationScope(hsamp,outputzrg_) )
    {
	delete chainexec_; chainexec_ = 0;
	return retError( tr("Could not set calculation scope."
		    "\nProbably there is not enough memory available.") );
    }

    if ( nrexecs_ < 2 )
	return MoreToDo();

    progresskeeper_.setMessage(
			tr("\nStarting new Volume Processing chunk %1-%2.\n")
			.arg( hsamp.start_.inl() ).arg( hsamp.stop_.inl() ) );
    progresskeeper_.setMessage( uiString::emptyString() );

    return MoreToDo();
}


#define mErrRet( msg ) { retError( msg ); return false; }

bool VolProc::ChainOutput::openOutput()
{
    VolProc::Step::CVolRef seisdp = chainexec_->getOutput();
    if ( !seisdp )
	mErrRet( tr("No output data available") )

    PtrMan<IOObj> ioobj = DBM().get( outid_ );
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
	DBM().setEntry( *ioobj );

    wrr_ = new SeisDataPackWriter( outid_, *seisdp );
    wrr_->setSelection( tkzs_.hsamp_, outputzrg_ );
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
    , dp_(dp)
    , work_(0)
{
    dp_.ref();
}

~ChainOutputStorer()
{
    dp_.unRef();
    if ( work_ )
	Threads::WorkManager::twm().removeWork( *work_ );
}

void startWork()
{
    SeisDataPackWriter& wrr = *co_.wrr_;
    if ( wrr.dataPack() != &dp_ )
	wrr.setNextDataPack( dp_ );
    else
	wrr.setSelection( dp_.sampling().hsamp_, wrr.zSampling() );

    if ( co_.nrexecs_ == co_.curexecnr_ )
	wrr.setProgressMeter( co_.progresskeeper_.forwardTo() );

    work_ = new Threads::Work( wrr, false );
    CallBack finishedcb( mCB(this,VolProc::ChainOutputStorer,workFinished) );
    Threads::WorkManager::twm().addWork( *work_, &finishedcb );
}

void workFinished( CallBacker* cb )
{
    const bool isfail = !Threads::WorkManager::twm().getWorkExitStatus( cb );
    if ( isfail )
    {
	errmsg_ = co_.wrr_->message();
	if ( errmsg_.isEmpty() )
	    errmsg_ = tr("Error during background write");
    }
    delete work_; work_ = 0;
    co_.reportFinished( *this );
}

    ChainOutput&	    co_;
    const RegularSeisDataPack& dp_;
    Threads::Work*	    work_;
    uiString		    errmsg_;

};

} // namespace VolProc


void VolProc::ChainOutput::startWriteChunk()
{
    const RegularSeisDataPack* dp = chainexec_->getOutput();
    if ( !dp )
	return;

    Threads::Locker slock( storerlock_ );
    storers_ += new ChainOutputStorer( *this, *dp );
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

    if ( storers_.isEmpty() || storers_[0]->work_ )
	return;

    storers_[0]->startWork();
}
