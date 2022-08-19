/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisimporter.h"

#include "seisbuf.h"
#include "seiscbvs.h"
#include "seisread.h"
#include "seisresampler.h"
#include "seisselection.h"
#include "seistrc.h"
#include "seiswrite.h"

#include "thread.h"
#include "threadwork.h"
#include "binidsorting.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "file.h"
#include "filepath.h"
#include "iostrm.h"
#include "ptrman.h"
#include "scaler.h"
#include "survinfo.h"
#include "uistrings.h"


SeisImporter::SeisImporter( SeisImporter::Reader* r, SeisTrcWriter& w,
			    Seis::GeomType gt )
    : Executor("Importing seismic data")
    , rdr_(r)
    , wrr_(w)
    , geomtype_(gt)
    , buf_(*new SeisTrcBuf(false))
    , trc_(*new SeisTrc)
    , state_( Seis::isPS(gt) ? ReadWrite : ReadBuf )
    , sortanal_(new BinIDSortingAnalyser(Seis::is2D(gt)))
    , prevbid_(*new BinID(mUdf(int),mUdf(int)))
    , lock_(*new Threads::ConditionVar)
    , maxqueuesize_( Threads::getNrProcessors()*100 )
{
    queueid_ = Threads::WorkManager::twm().addQueue(
					Threads::WorkManager::SingleThread,
					"SeisImporter");
    if ( rdr_ )
	wrr_.setCrFrom( rdr_->implName() );
}


SeisImporter::~SeisImporter()
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    buf_.deepErase();

    delete rdr_;
    delete sorting_;
    delete sortanal_;

    delete &buf_;
    delete &trc_;
    delete &prevbid_;
    delete &lock_;
}


uiString SeisImporter::uiMessage() const
{
    if ( !errmsg_.isEmpty() )
	return errmsg_;

    if ( hndlmsg_.isEmpty() )
	{ hndlmsg_ = tr("Importing from %1").arg(rdr_->name()); }
    return hndlmsg_;
}


od_int64 SeisImporter::nrDone() const
{
    return state_ == ReadBuf ? nrread_ : nrwritten_;
}


uiString SeisImporter::uiNrDoneText() const
{
    return state_ == ReadBuf ? tr("Traces read") : tr("Traces written");
}


od_int64 SeisImporter::totalNr() const
{
    return rdr_->totalNr();
}


bool SeisImporter::goImpl( od_ostream* strm, bool first, bool last, int delay )
{
    const bool res = Executor::goImpl( strm, first, last, delay );
    return res;
}


#define mDoRead(trc) \
    bool atend = false; \
    if ( rdr_->fetch(trc) ) \
	nrread_++; \
    else \
    { \
	errmsg_ = rdr_->errmsg_; \
	if ( !errmsg_.isEmpty() ) \
	    return ErrorOccurred(); \
	atend = true; \
    }


int SeisImporter::nextStep()
{
    if ( state_ == WriteBuf )
    {
	Threads::MutexLocker lock( lock_ );
	if ( !errmsg_.isEmpty() )
	    return ErrorOccurred();

	lock.unLock();

	if ( buf_.isEmpty() )
	    state_ = ReadWrite;
	else
	{
	    PtrMan<SeisTrc> trc = buf_.remove( ((int)0) );
	    return doWrite( *trc );
	}
    }

    if ( state_ == ReadWrite )
    {
	Threads::MutexLocker lock( lock_ );
	if ( !errmsg_.isEmpty() )
	    return ErrorOccurred();
	lock.unLock();

	mDoRead( trc_ );
	if ( !atend )
	{
	    const bool is2d = Seis::is2D(geomtype_);
	    if ( !is2d && !SI().isReasonable(trc_.info().binID()) )
	    {
		nrskipped_++;
		return MoreToDo();
	    }

	    return sortingOk(trc_) ? doWrite(trc_) : ErrorOccurred();
	}

	//Wait for queue to finish writing
	Threads::WorkManager::twm().emptyQueue( queueid_, true );

	//Check for errors
	lock.lock();
	if ( !errmsg_.isEmpty() )
	    return ErrorOccurred();
	lock.unLock();

	return Finished();
    }

    return readIntoBuf();
}


class SeisImporterWriterTask : public Task
{
public:
    SeisImporterWriterTask( SeisImporter& imp, SeisTrcWriter& writer,
			    const SeisTrc& trc )
	: importer_( imp )
	, writer_( writer )
	, trc_( trc )
    {}

    bool execute() override
    {
	uiString errmsg;
	if ( !writer_.put( trc_ ) )
	{
	    errmsg = writer_.errMsg();
	    if ( errmsg.isEmpty() )
	    {
		pErrMsg( "Need an error message from writer" );
		errmsg = uiStrings::phrCannotWrite( uiStrings::sTrace() );
	    }
	}

	importer_.reportWrite( errmsg );
	return errmsg.isEmpty();
    }


protected:

    SeisImporter&	importer_;
    SeisTrcWriter&	writer_;
    SeisTrc		trc_;
};


void SeisImporter::reportWrite( const uiString& errmsg )
{
    nrwritten_++;
    Threads::MutexLocker lock( lock_ );
    if ( !errmsg.isEmpty() )
    {
	errmsg_ = errmsg;
	Threads::WorkManager::twm().emptyQueue( queueid_, false );
	lock_.signal( true );
	return;
    }

    if ( Threads::WorkManager::twm().queueSize( queueid_ )<maxqueuesize_ )
	lock_.signal( true );
}



int SeisImporter::doWrite( SeisTrc& trc )
{
    Threads::MutexLocker lock( lock_ );
    while ( Threads::WorkManager::twm().queueSize( queueid_ )>maxqueuesize_ )
	lock_.wait();

    if ( !errmsg_.isEmpty() )
	return ErrorOccurred();

    lock.unLock();

    Task* task = new SeisImporterWriterTask( *this, wrr_, trc );
    Threads::WorkManager::twm().addWork(Threads::Work(*task,true), 0, queueid_,
				       false );
    return MoreToDo();
}


int SeisImporter::readIntoBuf()
{
    auto* trc = new SeisTrc;
    mDoRead( *trc )
    if ( atend )
    {
	delete trc;
	if ( nrread_ == 0 )
	{
	    errmsg_ = tr("No valid traces in input");
	    return ErrorOccurred();
	}

	state_ = buf_.isEmpty() ? ReadWrite : WriteBuf;
	return MoreToDo();
    }

    const bool is2d = Seis::is2D(geomtype_);
    if ( !is2d && !SI().isReasonable(trc->info().binID()) )
    {
	delete trc;
	nrskipped_++;
    }
    else
    {
	buf_.add( trc );
	if ( !sortingOk(*trc) )
	    return ErrorOccurred();
	if ( !sortanal_ )
	    state_ = WriteBuf;

	    // Check on max nr traces with same position
	if ( !Seis::isPS(geomtype_) && buf_.size() > 1000 )
	{
	    SeisTrc* btrc = buf_.get( buf_.size() - 1 );
	    const BinID trcbid( btrc->info().binID() );
	    const int trcnr = btrc->info().trcNr();
	    int nreq = 0;
	    for ( int idx=buf_.size()-2; idx!=-1; idx-- )
	    {
		btrc = buf_.get( idx );
		if ( (!is2d && btrc->info().binID() == trcbid)
		  || (is2d && btrc->info().trcNr() == trcnr) )
		    nreq++;
		else
		    break;
	    }
	    if ( nreq > 999 )
	    {
		errmsg_ = tr("Input contains too many (1000+) "
			     "identical positions");
		return ErrorOccurred();
	    }
	}
    }

    return MoreToDo();
}


bool SeisImporter::sortingOk( const SeisTrc& trc )
{
    const bool is2d = Seis::is2D(geomtype_);
    BinID bid( trc.info().binID() );
    if ( is2d )
    {
	bid.inl() = prevbid_.inl();
	if ( mIsUdf(bid.inl()) )
	    bid.inl() = 0;
	else if ( trc.info().new_packet )
	    bid.inl() = prevbid_.inl() + 1;
    }

    bool rv = true;
    if ( sorting_ )
    {
	if ( !sorting_->isValid(prevbid_,bid) )
	{
	    if ( is2d )
	    {
		errmsg_ = tr("Importing stopped at trace number: %1"
			     "\nbecause before this trace, the rule was:\n%2")
			.arg( toString(trc.info().trcNr()) )
                        .arg( sorting_->description() );
	    }
	    else
	    {
		errmsg_ = tr( "Importing stopped because trace position "
			      "found: %1 \nviolates previous trace "
			      "sorting:\n%2")
		        .arg( bid.toString() ).arg( sorting_->description() );
	    }

	    rv = false;
	}
    }
    else
    {
	if ( sortanal_->add(bid) )
	{
	    sorting_ = new BinIDSorting( sortanal_->getSorting() );
	    delete sortanal_; sortanal_ = 0;
	    if ( !is2d && !sorting_->inlSorted() )
	    {
		errmsg_ =
		tr("The input data is cross-line sorted.\n"
		   "This is not supported.\n\n"
		   "Did you switch the inline and crossline bytes?\n\n"
		   "If not, then for SEG-Y use the 'SEGY-scanned' import "
                   "method.\nFor Simple File consider re-sorting "
                   "with a text tool.\n");
		rv = false;
	    }
	}
	else if ( !sortanal_->errMsg().isEmpty() )
	{
	    errmsg_ = sortanal_->errMsg();
	    rv = false;
	}
    }

    prevbid_ = bid;
    return rv;
}


// SeisStdImporterReader

SeisStdImporterReader::SeisStdImporterReader( const SeisStoreAccess::Setup& su,
					      const char* nm )
    : rdr_(*new SeisTrcReader(su))
    , name_(nm)
{
    totalnr_ = rdr_.expectedNrTraces();
}


SeisStdImporterReader::SeisStdImporterReader( const IOObj& ioobj,
					      const char* nm )
    : SeisStdImporterReader(SeisStoreAccess::Setup(ioobj,nullptr),nm)
{
}


SeisStdImporterReader::~SeisStdImporterReader()
{
    delete resampler_;
    delete scaler_;
    delete &rdr_;
}


const char* SeisStdImporterReader::implName() const
{
    return rdr_.ioObj() ? rdr_.ioObj()->fullUserExpr() : "";
}


int SeisStdImporterReader::totalNr() const
{
    return totalnr_;
}


void SeisStdImporterReader::setResampler( SeisResampler* r )
{
    delete resampler_; resampler_ = r;
}


void SeisStdImporterReader::setScaler( Scaler* s )
{
    delete scaler_; scaler_ = s;
}


void SeisStdImporterReader::setSelData( Seis::SelData* sd )
{
    rdr_.setSelData( sd );
}


bool SeisStdImporterReader::fetch( SeisTrc& trc )
{
    while ( true )
    {
	int rv = 2;
	while ( rv != 1 )
	{
	    rv = rdr_.get(trc.info());
	    if ( rv < 1 )
	    {
		if ( rv < 0 ) errmsg_ = rdr_.errMsg();
		return false;
	    }
	}
	if ( !rdr_.get(trc) )
	{
	    errmsg_ = rdr_.errMsg();
	    return false;
	}

	if ( remnull_ && trc.isNull() )
	    continue;
	else if ( resampler_ )
	{
	    SeisTrc* outtrc = resampler_->get( trc );
	    if ( !outtrc )
		continue;
	    if ( outtrc != &trc )
		trc = *outtrc;
	}

	if ( scaler_ )
	    trc.data().scale( *scaler_ );

	break;
    }

    return true;
}
