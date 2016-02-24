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
#include "keystrs.h"
#include "moddepmgr.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "veldesc.h"
#include "odsysmem.h"
#include "threadwork.h"


VolProc::ChainOutput::ChainOutput()
    : Executor("Volume Processing Output")
    , cs_(true)
    , chain_(0)
    , chainexec_(0)
    , wrr_(0)
    , neednextchunk_(true)
    , nrexecs_(-1)
    , curexecnr_(-1)
    , calculating_(true)
    , storererr_(false)
{
    msg_ = tr("Reading Volume Processing Specification");
}


VolProc::ChainOutput::~ChainOutput()
{
    chain_->unRef();
    delete chainexec_;
    delete wrr_;
    deepErase( storers_ );
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


od_int64 VolProc::ChainOutput::nrDone() const
{
    if ( curexecnr_ < 0 )
	return 0;
    if ( calculating_ )
	return curexecnr_*100
	     + (chainexec_ ? chainexec_->nrDone() : 0) / nrexecs_;

    Threads::Locker slock( storerlock_ );
    if ( storers_.isEmpty() )
	return 0;
    return wrr_ ? wrr_->nrDone() : 0;
}


od_int64 VolProc::ChainOutput::totalNr() const
{
    if ( curexecnr_ < 0 )
	return -1;
    if ( calculating_ )
	return 100;

    Threads::Locker slock( storerlock_ );
    if ( storers_.isEmpty() )
	return -1;
    return wrr_ ? wrr_->totalNr() : -1;
}


uiString VolProc::ChainOutput::uiNrDoneText() const
{
    if ( calculating_ && chainexec_ )
	return chainexec_->uiNrDoneText();
    else if ( wrr_ )
	return wrr_->uiNrDoneText();
    return uiString::emptyString();
}


int VolProc::ChainOutput::nextStep()
{
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

    if ( calculating_ )
    {
	int res = chainexec_->doStep();
	if ( res < 0 )
	    { msg_ = chainexec_->uiMessage(); return ErrorOccurred(); }

	if ( res == 0 )
	{
	    if ( !wrr_ && !openOutput() )
		return ErrorOccurred();

	    neednextchunk_ = true;
	    startWriteChunk();
	}

	msg_ = chainexec_->uiMessage();
	return MoreToDo();
    }

    slock.reLock();
    if ( !storers_.isEmpty() )
    {
	msg_ = wrr_->uiMessage();
	slock.unlockNow();
	Threads::sleep( 0.1 );
	return MoreToDo();
    }

    // no calculations going on, no storers left ...
    return Finished();
}


int VolProc::ChainOutput::getChain()
{
    if ( chainid_.isEmpty() )
    {
	msg_ = tr( "No Volume Processing ID specified" );
	return ErrorOccurred();
    }

    PtrMan<IOObj> ioobj = IOM().get( chainid_ );
    if ( !ioobj )
    {
	msg_ = uiStrings::phrCannotFind(
		tr("Volume Processing with id: %1").arg(chainid_) );
	return ErrorOccurred();
    }

    chain_ = new Chain; chain_->ref();
    if ( !VolProcessingTranslator::retrieve(*chain_,ioobj,msg_) )
	return ErrorOccurred();
    else if ( chain_->nrSteps() < 1 )
    {
	msg_ = tr("Empty Volume Processing Chain - nothing to do.");
	return Finished();
    }

    const Step& step0 = *chain_->getStep(0);
    if ( step0.needsInput() )
    {
	msg_ = tr("First step in Volume Processing Chain (%1) requires input."
		"\nIt can thus not be first.").arg( step0.userName() );
	return ErrorOccurred();
    }

    msg_ = tr("Creating Volume Processor");
    return MoreToDo();
}


int VolProc::ChainOutput::setupChunking()
{
    chainexec_ = new VolProc::ChainExecutor( *chain_ );

    const float zstep = chain_->getZStep();
    outputzrg_ = StepInterval<int>( mNINT32(cs_.zsamp_.start/zstep),
				 mNINT32(cs_.zsamp_.stop/zstep),
				 mMIN(mNINT32(cs_.zsamp_.step/zstep),1) );

    // We will be writing while a new chunk will be calculated
    // Thus, we need to keep the output datapack alive while the next chunk
    // is calculated. Therefore, lets double the computed mem need:

    od_uint64 nrbytes = 2 * chainexec_->computeMaximumMemoryUsage( cs_.hsamp_,
								   outputzrg_ );
    od_int64 totmem, freemem; OD::getSystemMemory( totmem, freemem );
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
	{
	    msg_ = tr("Processing aborted; not enough available memory.");
	    return ErrorOccurred();
	}
    }
    if ( needsplit )
    {
	nrexecs_ = (int)(nrbytes / freemem) + 1;
	if ( nrexecs_ > cs_.hsamp_.nrLines() )
	    nrexecs_ = cs_.hsamp_.nrLines(); // and pray!
    }

    msg_ = tr("Allocating memory");
    neednextchunk_ = true;
    return MoreToDo();
}


int VolProc::ChainOutput::setNextChunk()
{
    neednextchunk_ = false;
    curexecnr_++;
    if ( curexecnr_ >= nrexecs_ )
    {
	calculating_ = false;
	return MoreToDo();
    }

    if ( curexecnr_ > 0 )
    {
	delete chainexec_;
	chainexec_ = new VolProc::ChainExecutor( *chain_ );
    }

    const TrcKeySampling hsamp( cs_.hsamp_.getLineChunk(nrexecs_,curexecnr_) );
    if ( !chainexec_->setCalculationScope(hsamp,outputzrg_) )
    {
	msg_ = tr("Could not set calculation scope."
		"\nProbably there is not enough memory available.");
	return ErrorOccurred();
    }

    msg_ = chainexec_->uiMessage();
    return MoreToDo();
}


bool VolProc::ChainOutput::openOutput()
{
    ConstDataPackRef<RegularSeisDataPack> seisdp = chainexec_->getOutput();
    if ( !seisdp )
	{ msg_ = tr("No output data available"); return false; }

    const VelocityDesc* vd = chain_->getVelDesc();
    ConstPtrMan<VelocityDesc> veldesc = vd ? new VelocityDesc( *vd ) : 0;

    PtrMan<IOObj> ioobj = IOM().get( outid_ );
    if ( !ioobj )
    {
	msg_ = uiStrings::phrCannotFind( tr("output cube ID in database") );
	return false;
    }

    bool docommit = false;
    VelocityDesc omfdesc;
    const bool hasveldesc = omfdesc.usePar( ioobj->pars() );
    if ( veldesc )
    {
	if ( !hasveldesc || omfdesc!=*veldesc )
	{
	    veldesc->fillPar( ioobj->pars() );
	    docommit = true;
	}
    }
    else if ( hasveldesc )
    {
	VelocityDesc::removePars( ioobj->pars() );
	docommit = true;
    }

    if ( docommit )
    {
	if ( !IOM().commitChanges( *ioobj ) )
	{
	    msg_ = uiStrings::phrCannotWriteDBEntry( ioobj->uiName() );
	    return false;
	}
    }

    wrr_ = new SeisDataPackWriter( outid_, *seisdp );
    wrr_->setSelection( cs_.hsamp_, outputzrg_ );
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
	msg_ = storer.errmsg_;
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
