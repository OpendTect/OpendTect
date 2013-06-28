/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

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
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "iostrm.h"
#include "ptrman.h"
#include "scaler.h"
#include "survinfo.h"


SeisImporter::SeisImporter( SeisImporter::Reader* r, SeisTrcWriter& w,
			    Seis::GeomType gt )
    	: Executor("Importing seismic data")
    	, rdr_(r)
    	, wrr_(w)
    	, geomtype_(gt)
    	, buf_(*new SeisTrcBuf(false))
    	, trc_(*new SeisTrc)
    	, state_( Seis::isPS(gt) ? ReadWrite : ReadBuf )
    	, nrread_(0)
    	, nrwritten_(0)
    	, nrskipped_(0)
    	, postproc_(0)
	, sortanal_(new BinIDSortingAnalyser(Seis::is2D(gt)))
	, sorting_(0)
	, prevbid_(*new BinID(mUdf(int),mUdf(int)))
        , lock_(*new Threads::ConditionVar)
        , maxqueuesize_( Threads::getNrProcessors()*100 )
{
    queueid_ = Threads::WorkManager::twm().addQueue(
					Threads::WorkManager::SingleThread,
					"SeisImporter");
}


SeisImporter::~SeisImporter()
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );

    buf_.deepErase();

    delete rdr_;
    delete sorting_;
    delete sortanal_;
    delete postproc_;

    delete &buf_;
    delete &trc_;
    delete &prevbid_;
    delete &lock_;
}


const char* SeisImporter::message() const
{
    if ( postproc_ ) return postproc_->message();
    if ( !errmsg_.isEmpty() )
	return errmsg_;

    if ( hndlmsg_.isEmpty() )
	{ hndlmsg_ = "Importing from "; hndlmsg_ += rdr_->name(); }
    return hndlmsg_;
}


od_int64 SeisImporter::nrDone() const
{
    if ( postproc_ ) return postproc_->nrDone();
    return state_ == ReadBuf ? nrread_ : nrwritten_;
}


const char* SeisImporter::nrDoneText() const
{
    if ( postproc_ ) return postproc_->nrDoneText();
    return state_ == ReadBuf ? "Traces read" : "Traces written";
}


od_int64 SeisImporter::totalNr() const
{
    if ( postproc_ ) return postproc_->totalNr();
    return rdr_->totalNr();
}


#define mDoRead(trc) \
    bool atend = false; \
    if ( rdr_->fetch(trc) ) \
	nrread_++; \
    else \
    { \
	errmsg_ = rdr_->errmsg_; \
	if ( !errmsg_.isEmpty() ) \
	    return Executor::ErrorOccurred(); \
	atend = true; \
    }


int SeisImporter::nextStep()
{
    if ( postproc_ ) return postproc_->doStep();

    if ( state_ == WriteBuf )
    {
	Threads::MutexLocker lock( lock_ );
	if ( errmsg_.str() )
	    return Executor::ErrorOccurred();

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
	if ( errmsg_.str() )
	    return Executor::ErrorOccurred();
	lock.unLock();

	mDoRead( trc_ );
	if ( !atend )
	{
	    const bool is2d = Seis::is2D(geomtype_);
	    if ( !is2d && !SI().isReasonable(trc_.info().binid) )
	    {
		nrskipped_++;
		return Executor::MoreToDo();
	    }

	    return sortingOk(trc_) ? doWrite(trc_) : Executor::ErrorOccurred();
	}

	//Wait for queue to finish writing
	Threads::WorkManager::twm().emptyQueue( queueid_, true );

	//Check for errors
	lock.lock();
	if ( errmsg_.str() )
	    return Executor::ErrorOccurred();
	lock.unLock();
	
	postproc_ = mkPostProc();
	if ( !errmsg_.isEmpty() )
	    return Executor::ErrorOccurred();
	return postproc_ ? Executor::MoreToDo() : Executor::Finished();
    }

    return readIntoBuf();
}


bool SeisImporter::needInlCrlSwap() const
{
    return !Seis::is2D(geomtype_) && sorting_ && !sorting_->inlSorted();
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

    bool execute()
    {
	BufferString errmsg;
	if ( !writer_.put( trc_ ) )
	{
	    errmsg = writer_.errMsg();
	    if ( errmsg.isEmpty() )
	    {
		pErrMsg( "Need an error message from writer" );
		errmsg = "Cannot write trace";
	    }
	}

	importer_.reportWrite( errmsg.str() );
	return !errmsg.str();
    }


protected:

    SeisImporter&	importer_;
    SeisTrcWriter&	writer_;
    SeisTrc		trc_;
};


void SeisImporter::reportWrite( const char* errmsg )
{
    nrwritten_++;
    Threads::MutexLocker lock( lock_ );
    if ( errmsg )
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
    if ( needInlCrlSwap() )
	Swap( trc.info().binid.inl, trc.info().binid.crl );

    Threads::MutexLocker lock( lock_ );
    while ( Threads::WorkManager::twm().queueSize( queueid_ )>maxqueuesize_ )
	lock_.wait();

    if ( errmsg_.str() )
	return Executor::ErrorOccurred();

    lock.unLock();

    Task* task = new SeisImporterWriterTask( *this, wrr_, trc );
    Threads::WorkManager::twm().addWork(Threads::Work(*task,true), 0, queueid_,
				       false );
    return Executor::MoreToDo();
}


int SeisImporter::readIntoBuf()
{
    SeisTrc* trc = new SeisTrc;
    mDoRead( *trc )
    if ( atend )
    {
	delete trc;
	if ( nrread_ == 0 )
	{
	    errmsg_ = "No valid traces in input";
	    return Executor::ErrorOccurred();
	}

	state_ = buf_.isEmpty() ? ReadWrite : WriteBuf;
	return Executor::MoreToDo();
    }

    const bool is2d = Seis::is2D(geomtype_);
    if ( !is2d && !SI().isReasonable(trc->info().binid) )
    {
	delete trc;
	nrskipped_++;
    }
    else
    {
	buf_.add( trc );
	if ( !sortingOk(*trc) )
	    return Executor::ErrorOccurred();
	if ( !sortanal_ )
	    state_ = WriteBuf;

	    // Check on max nr traces with same position
	if ( !Seis::isPS(geomtype_) && buf_.size() > 1000 )
	{
	    SeisTrc* btrc = buf_.get( buf_.size() - 1 );
	    const BinID trcbid( btrc->info().binid );
	    const int trcnr = btrc->info().nr;
	    int nreq = 0;
	    for ( int idx=buf_.size()-2; idx!=-1; idx-- )
	    {
		btrc = buf_.get( idx );
		if ( (!is2d && btrc->info().binid == trcbid)
		  || (is2d && btrc->info().nr == trcnr) )
		    nreq++;
		else
		    break;
	    }
	    if ( nreq > 999 )
	    {
		errmsg_ = "Input contains too many (1000+) identical positions";
		return Executor::ErrorOccurred();
	    }
	}
    }

    return Executor::MoreToDo();
}


bool SeisImporter::sortingOk( const SeisTrc& trc )
{
    const bool is2d = Seis::is2D(geomtype_);
    BinID bid( trc.info().binid );
    if ( is2d )
    {
	bid.crl = trc.info().nr;
	bid.inl = prevbid_.inl;
	if ( mIsUdf(bid.inl) )
	    bid.inl = 0;
	else if ( trc.info().new_packet )
	    bid.inl = prevbid_.inl + 1;
    }

    bool rv = true;
    if ( sorting_ )
    {
	if ( !sorting_->isValid(prevbid_,bid) )
	{
	    if ( is2d )
	    {
		errmsg_ = "Importing stopped at trace number: ";
		errmsg_ += trc.info().nr;
		errmsg_ += "\nbecause before this trace, the rule was:\n";
	    }
	    else
	    {
		char buf[30]; bid.fill( buf );
		errmsg_ = "Importing stopped because trace position found: ";
		errmsg_ += buf;
		errmsg_ += "\nviolates previous trace sorting:\n";
	    }
	    errmsg_ += sorting_->description();
	    rv = false;
	}
    }
    else
    {
	if ( sortanal_->add(bid) )
	{
	    sorting_ = new BinIDSorting( sortanal_->getSorting() );
	    delete sortanal_; sortanal_ = 0;
	    if ( !is2d && Seis::isPS(geomtype_) && !sorting_->inlSorted() )
	    {
		errmsg_ = "The input data is cross-line sorted.\n"
		    "This is not supported for Pre-stack data.\n"
		    "Did you switch the inline and crossline bytes?\n"
		    "If not, there is a 'SEG-Y Pre-stack scanned'\n"
		    "that will read cross-line sorted data.";
		rv = false;
	    }
	}
	else if ( sortanal_->errMsg() )
	{
	    errmsg_ = sortanal_->errMsg();
	    rv = false;
	}
    }

    prevbid_ = bid;
    return rv;
}


class SeisInlCrlSwapper : public Executor
{
public:

SeisInlCrlSwapper( const char* inpfnm, IOObj* out, int nrtrcs )
    : Executor( "Swapping In/X-line" )
    , tri_(CBVSSeisTrcTranslator::getInstance())
    , targetfnm_(inpfnm)
    , wrr_(0)
    , out_(out)
    , nrdone_(0)
    , totnr_(nrtrcs)
{
    if ( !tri_->initRead(new StreamConn(inpfnm,Conn::Read)) )
	{ errmsg_ = tri_->errMsg(); return; }
    geom_ = &tri_->readMgr()->info().geom_;
    linenr_ = geom_->start.crl;
    trcnr_ = geom_->start.inl - geom_->step.inl;

    wrr_ = new SeisTrcWriter( out_ );
    if ( wrr_->errMsg() )
	{ errmsg_ = wrr_->errMsg(); return; }

    nrdone_++;
}

~SeisInlCrlSwapper()
{
    delete tri_;
    delete wrr_;
    delete out_;
}

const char* message() const
{
    return errmsg_.isEmpty() ? "Re-sorting traces" : ((const char*)errmsg_);
}
const char* nrDoneText() const	{ return "Traces handled"; }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totnr_; }

int nextStep()
{
    if ( !errmsg_.isEmpty() )
	return Executor::ErrorOccurred();

    trcnr_ += geom_->step.inl;
    if ( trcnr_ > geom_->stop.inl )
    {
	linenr_ += geom_->step.crl;
	if ( linenr_ > geom_->stop.crl )
	    return doFinal();
	else
	    trcnr_ = geom_->start.inl;
    }

    if ( tri_->goTo(BinID(trcnr_,linenr_)) )
    {
	if ( !tri_->read(trc_) )
	{
	    errmsg_ = "Cannot read ";
	    errmsg_ += linenr_; errmsg_ += "/"; errmsg_ += trcnr_;
	    errmsg_ += ":\n"; errmsg_ += tri_->errMsg();
	    return Executor::ErrorOccurred();
	}

	Swap( trc_.info().binid.inl, trc_.info().binid.crl );
	trc_.info().coord = SI().transform( trc_.info().binid );

	if ( !wrr_->put(trc_) )
	{
	    errmsg_ = "Cannot write ";
	    errmsg_ += linenr_; errmsg_ += "/"; errmsg_ += trcnr_;
	    errmsg_ += ":\n"; errmsg_ += wrr_->errMsg();
	    return Executor::ErrorOccurred();
	}
	nrdone_++;
    }
    return Executor::MoreToDo();
}

int doFinal()
{
    delete wrr_; wrr_ = 0;
    const BufferString tmpfnm( out_->fullUserExpr(false) );

    if ( nrdone_ < 1 )
    {
	errmsg_ = "No traces written during re-sorting.\n";
	errmsg_ += "The imported cube remains to have swapped in/crosslines";
	File::remove( tmpfnm );
	return Executor::ErrorOccurred();
    }

    if ( !File::remove(targetfnm_)
      || !File::rename(tmpfnm,targetfnm_) )
    {
	errmsg_ = "Your input data is cross-line sorted.\n"
	    	  "OpendTect has re-sorted to in-line sorting.\n"
		  "Unfortunately, the last step in the process failed.\n"
		  "Could not rename: '";
	errmsg_ += tmpfnm; errmsg_ += "'\nto: '"; errmsg_ += targetfnm_;
	errmsg_ += "'\nPlease do this yourself, by hand.";
	return Executor::ErrorOccurred();
    }

    return Executor::Finished();
}

    BufferString		targetfnm_;
    CBVSSeisTrcTranslator*	tri_;
    SeisTrcWriter*		wrr_;
    const CBVSInfo::SurvGeom*	geom_;
    IOObj*			out_;
    SeisTrc			trc_;

    BufferString		errmsg_;
    int				nrdone_;
    int				totnr_;
    int				linenr_;
    int				trcnr_;

};


Executor* SeisImporter::mkPostProc()
{
    errmsg_ = "";
    if ( needInlCrlSwap() )
    {
	const IOObj* targetioobj = wrr_.ioObj();
	mDynamicCastGet(const IOStream*,targetiostrm,targetioobj)
	if ( !targetiostrm )
	{
	    errmsg_ = "Your data was loaded with swapped inline/crossline\n"
		      "Please use the batch program 'cbvs_swap_inlcrl' now\n";
	    return 0;
	}

	FilePath fp( targetiostrm->getExpandedName(false) );
	const BufferString targetfnm( fp.fullPath() );
	FilePath fptemp( FilePath::getTempName("cbvs") );
	fp.setFileName( fptemp.fileName() );
	const BufferString tmpfnm( fp.fullPath() );
	IOStream* tmpiostrm = (IOStream*)targetiostrm->clone();
	tmpiostrm->setFileName( tmpfnm );
	return new SeisInlCrlSwapper( targetfnm, tmpiostrm, nrwritten_ );
    }
    return 0;
}


SeisStdImporterReader::SeisStdImporterReader( const IOObj& ioobj,
					      const char* nm )
    : rdr_(*new SeisTrcReader(&ioobj))
    , name_(nm)
    , remnull_(false)
    , resampler_(0)
    , scaler_(0)
{
}


SeisStdImporterReader::~SeisStdImporterReader()
{
    delete resampler_;
    delete scaler_;
    delete &rdr_;
}


int SeisStdImporterReader::totalNr() const
{
    return rdr_.selData() ? rdr_.selData()->expectedNrTraces() : -1;
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
