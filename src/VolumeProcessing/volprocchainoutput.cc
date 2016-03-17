/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/


#include "volprocchainoutput.h"

#include "volprocchainexec.h"
#include "volproctrans.h"
#include "seisdatapackwriter.h"
#include "ioman.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "veldesc.h"
#include "odsysmem.h"
#include "threadwork.h"
#include "progressmeterimpl.h"


VolProc::ChainOutput::ChainOutput()
    : Executor("Volume Processing Output")
    , cs_(true)
    , chain_(0)
    , chainexec_(0)
    , wrr_(0)
    , neednextchunk_(true)
    , nrexecs_(-1)
    , curexecnr_(-1)
    , storererr_(false)
    , progresskeeper_(*new ProgressRecorder)
{
    progressmeter_ = &progresskeeper_;
    progresskeeper_.setMessage( tr("Reading Volume Processing Specification") );
}


VolProc::ChainOutput::~ChainOutput()
{
    chain_->unRef();
    delete chainexec_; chainexec_ = 0;
    delete wrr_;
    deepErase( storers_ );
    delete &progresskeeper_;
}


void VolProc::ChainOutput::usePar( const IOPar& iop )
{
    iop.get( VolProcessingTranslatorGroup::sKeyChainID(), chainid_ );

    PtrMan<IOPar> subselpar = iop.subselect(
	    IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    if ( subselpar )
       cs_.usePar( *subselpar );

    iop.get( "Output.0.Seismic.ID", outid_ );
}


void VolProc::ChainOutput::setProgressMeter( ProgressMeter* pm )
{
    progresskeeper_.setForwardTo( pm );
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
    chainexec_->setProgressMeter( &progresskeeper_ );
}


int VolProc::ChainOutput::retError( const uiString& msg )
{
    progresskeeper_.setMessage( msg );
    return ErrorOccurred();
}


int VolProc::ChainOutput::retMoreToDo()
{
    if ( !chainexec_ && wrr_ )
	progresskeeper_.setFrom( *wrr_ );

    return MoreToDo();
}


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
#ifdef __debug__
    uiString ret = progresskeeper_.nrDoneText();
    if ( FixedString(ret.getFullString())=="Nr Done" )
	ret = toUiString("# done");
    return ret;
#else
    return progresskeeper_.nrDoneText();
#endif
}


uiString VolProc::ChainOutput::uiMessage() const
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

	    neednextchunk_ = true;
	    startWriteChunk();
	}

	return retMoreToDo();
    }

    slock.reLock();
    if ( !storers_.isEmpty() )
    {
	slock.unlockNow();
	Threads::sleep( 0.1 );
	return retMoreToDo();
    }

    // no calculations going on, no storers left ...
    return Finished();
}


int VolProc::ChainOutput::getChain()
{
    if ( chainid_.isEmpty() )
	return retError( tr("No Volume Processing ID specified") );

    PtrMan<IOObj> ioobj = IOM().get( chainid_ );
    if ( !ioobj )
	return retError( uiStrings::phrCannotFind(
		tr("Volume Processing with id: %1").arg(chainid_) ) );

    chain_ = new Chain; chain_->ref();
    uiString errmsg;
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj,errmsg) )
    {
	progresskeeper_.setMessage( errmsg );
	return ErrorOccurred();
    }
    else if ( chain_->nrSteps() < 1 )
	return retError( tr("Empty Volume Processing Chain - nothing to do.") );

    const Step& step0 = *chain_->getStep(0);
    if ( step0.needsInput() )
	return retError(
		tr("First step in Volume Processing Chain (%1) requires input."
		    "\nIt can thus not be first.").arg( step0.userName() ) );

    progresskeeper_.setMessage( tr("Creating Volume Processor") );
    return MoreToDo();
}


int VolProc::ChainOutput::setupChunking()
{
    createNewChainExec();

    const float zstep = chain_->getZStep();
    outputzrg_ = StepInterval<int>( mNINT32(cs_.zsamp_.start/zstep),
				 mNINT32(Math::Ceil(cs_.zsamp_.stop/zstep)),
			   mMAX(mNINT32(Math::Ceil(cs_.zsamp_.step/zstep)),1) );
			   //real -> index, outputzrg_ is the index of z-samples

    // We will be writing while a new chunk will be calculated
    // Thus, we need to keep the output datapack alive while the next chunk
    // is calculated. Therefore, lets double the computed mem need:

    od_uint64 nrbytes = 2 * chainexec_->computeMaximumMemoryUsage( cs_.hsamp_,
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
	if ( nrexecs_ > cs_.hsamp_.nrLines() )
	    nrexecs_ = cs_.hsamp_.nrLines(); // and pray!
    }

    neednextchunk_ = true;
    return MoreToDo();
}


int VolProc::ChainOutput::setNextChunk()
{
    neednextchunk_ = false;
    curexecnr_++;
    if ( curexecnr_ >= nrexecs_ )
    {
	delete chainexec_; chainexec_ = 0;
	return retMoreToDo();
    }

    if ( curexecnr_ > 0 )
	createNewChainExec();

    const TrcKeySampling hsamp( cs_.hsamp_.getLineChunk(nrexecs_,curexecnr_) );

    if ( !chainexec_->setCalculationScope(hsamp,outputzrg_) )
    {
	delete chainexec_; chainexec_ = 0;
	return retError( tr("Could not set calculation scope."
		    "\nProbably there is not enough memory available.") );
    }

    if ( nrexecs_ < 2 )
	return retMoreToDo();

    progresskeeper_.setMessage(
	    tr("\nStarting new Volume Processing chunk %1-%2.\n")
	    .arg( hsamp.start_.inl() ).arg( hsamp.stop_.inl() ) );
    return MoreToDo();
}


#define mErrRet( msg ) { retError( msg ); return false; }

bool VolProc::ChainOutput::openOutput()
{
    const RegularSeisDataPack* seisdp = chainexec_->getOutput();
    if ( !seisdp )
	mErrRet( tr("No output data available") )
    DPM( DataPackMgr::SeisID() ).obtain( seisdp->id() );
    ConstDataPackRef<RegularSeisDataPack> seisdpcref = seisdp;

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

    wrr_ = new SeisDataPackWriter( outid_, *seisdp );
    wrr_->setSelection( cs_.hsamp_, outputzrg_ );
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
    DPM( DataPackMgr::SeisID() ).obtain( dp_.id() );
}

~ChainOutputStorer()
{
    DPM( DataPackMgr::SeisID() ).release( dp_.id() );
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

    work_ = new Threads::Work( wrr, false );
    CallBack finishedcb( mCB(this,VolProc::ChainOutputStorer,workFinished) );
    Threads::WorkManager::twm().addWork( *work_, &finishedcb );
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
	if ( chainexec_ )
	    chainexec_->setProgressMeter( 0 );
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
