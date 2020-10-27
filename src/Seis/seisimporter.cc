/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2006
-*/


#include "seisimporter.h"

#include "seisbuf.h"
#include "seiscbvs.h"
#include "seisprovider.h"
#include "seisresampler.h"
#include "seisseldata.h"
#include "seisstorer.h"
#include "seistrc.h"

#include "binidsorting.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "file.h"
#include "iostrm.h"
#include "ptrman.h"
#include "scaler.h"
#include "survinfo.h"
#include "uistrings.h"


SeisImporter::SeisImporter( SeisImporter::Reader* r, Storer& s,
			    Seis::GeomType gt )
	: Executor("Importing seismic data")
	, rdr_(r)
	, storer_(s)
	, geomtype_(gt)
	, buf_(*new SeisTrcBuf(false))
	, trc_(*new SeisTrc)
	, state_( Seis::isPS(gt) ? ReadWrite : ReadBuf )
	, nrread_(0)
	, nrwritten_(0)
	, nrskipped_(0)
	, sortanal_(new BinIDSortingAnalyser(Seis::is2D(gt)))
	, sorting_(0)
	, prevbid_(*new BinID(mUdf(int),mUdf(int)))
{
    if ( rdr_ )
	storer_.setCrFrom( rdr_->implName() );
}


SeisImporter::~SeisImporter()
{
    buf_.deepErase();

    delete rdr_;
    delete sorting_;
    delete sortanal_;

    delete &buf_;
    delete &trc_;
    delete &prevbid_;
}


uiString SeisImporter::message() const
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


int SeisImporter::Finished()
{
    if ( nrskipped_ > 0 )
	errmsg_.appendPhrase(
		tr("During import, %1 traces were rejected").arg(nrskipped_) );

    return 0;
}


uiString SeisImporter::nrDoneText() const
{
    return state_ == ReadBuf ? tr("Traces read") : tr("Traces written");
}


od_int64 SeisImporter::totalNr() const
{
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
    if ( state_ == WriteBuf )
    {
	if ( !errmsg_.isEmpty() )
	    return Executor::ErrorOccurred();

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
	if ( !errmsg_.isEmpty() )
	    return Executor::ErrorOccurred();

	mDoRead( trc_ );

	if ( !atend )
	{
	    const bool is2d = Seis::is2D(geomtype_);
	    if ( !is2d && !SI().isReasonable(trc_.info().binID()) )
	    {
		nrskipped_++;
		return Executor::MoreToDo();
	    }

	    return sortingOk(trc_) ? doWrite(trc_) : Executor::ErrorOccurred();
	}

	if ( !errmsg_.isEmpty() )
	    return Executor::ErrorOccurred();

	return Finished();
    }

    return readIntoBuf();
}


int SeisImporter::doWrite( SeisTrc& trc )
{
    if ( !errmsg_.isEmpty() )
    {
	return Executor::ErrorOccurred();
    }

    const auto uirv = storer_.put( trc );
    nrwritten_++;
    if ( !uirv.isOK() )
	errmsg_ = uirv;
    return Executor::MoreToDo();
}


int SeisImporter::readIntoBuf()
{
    SeisTrc* trc = new SeisTrc;
    mDoRead( *trc )
    const bool is2d = Seis::is2D(geomtype_);

    if ( atend )
    {
	delete trc;
	if ( nrread_ == 0 )
	{
	    uiString& msg = errmsg_;
	    const StepInterval<int> inlrg( SI().inlRange() );
	    const StepInterval<int> crlrg( SI().crlRange() );
	    msg.appendPhrase( tr("No valid traces in input") );
	    msg.appendPhrase( tr("The trace range is outside survey range:"));
	    msg.appendPhrase( tr("Inline Range: %1 - %2")
				       .arg(inlrg.start).arg(inlrg.stop) );
	    msg.appendPhrase( tr("Crossline Range: %1 - %2")
				       .arg(crlrg.start).arg(crlrg.stop) );
	    return Executor::ErrorOccurred();
	}

	state_ = buf_.isEmpty() ? ReadWrite : WriteBuf;
	return Executor::MoreToDo();
    }

    if ( !is2d && !SI().isReasonable(trc->info().binID()) )
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
		return Executor::ErrorOccurred();
	    }
	}
    }

    return Executor::MoreToDo();
}


bool SeisImporter::sortingOk( const SeisTrc& trc )
{
    const bool is2d = Seis::is2D(geomtype_);
    BinID bid( trc.info().binID() );
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
		errmsg_ = tr("Importing stopped because trace position "
			     "found (%1)\nviolates previous trace sorting (%2)")
		        .arg( bid.toString() )
			.arg( sorting_->description() );
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



SeisStdImporterReader::SeisStdImporterReader( const IOObj& ioobj,
					      const char* nm,
					      bool forceFPdata )
    : prov_(0)
    , name_(nm)
    , remnull_(false)
    , resampler_(0)
    , scaler_(0)
{
    uiRetVal uirv;
    prov_ = Seis::Provider::create( ioobj, &uirv );
    if ( !prov_ )
	errmsg_ = uirv;
    else if ( forceFPdata )
	prov_->forceFPData();
}


SeisStdImporterReader::~SeisStdImporterReader()
{
    delete resampler_;
    delete scaler_;
    delete prov_;
}


const char* SeisStdImporterReader::implName() const
{
    return prov_ ? prov_->name().str() : OD::EmptyString();
}


od_int64 SeisStdImporterReader::totalNr() const
{
    return prov_ ? prov_->totalNr() : 0;
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
    if ( prov_ )
	prov_->setSelData( sd );
}


bool SeisStdImporterReader::fetch( SeisTrc& trc )
{
    if ( !prov_ ) return false;

    while ( true )
    {
	const uiRetVal uirv = prov_->getNext( trc );
	if ( !uirv.isOK() )
	{
	    if ( !isFinished(uirv) )
		errmsg_ = uirv;
	    else
		errmsg_.setEmpty();
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
