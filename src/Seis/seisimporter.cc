/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2006
-*/

static const char* rcsID = "$Id: seisimporter.cc,v 1.5 2006-12-11 10:45:40 cvsbert Exp $";

#include "seisimporter.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "binidsorting.h"
#include "ptrman.h"
#include "errh.h"


SeisImporter::SeisImporter( SeisImporter::Reader* r, SeisTrcWriter& w,
			    Seis::GeomType gt )
    	: Executor("Importing seismic data")
    	, rdr_(r)
    	, wrr_(w)
    	, geomtype_(gt)
    	, buf_(*new SeisTrcBuf)
    	, trc_(*new SeisTrc)
    	, state_(ReadBuf)
    	, nrread_(0)
    	, nrwritten_(0)
    	, postproc_(0)
    	, removenulltrcs_(false)
	, sortanal_(new BinIDSortingAnalyser(Seis::is2D(gt)))
	, sorting_(0)
	, prevbinid_(*new BinID(mUdf(int),mUdf(int)))
	, sort2ddir_(0)
{
}


SeisImporter::~SeisImporter()
{
    buf_.deepErase();
    delete postproc_;
    delete rdr_;
    delete sorting_;
    delete sortanal_;
    delete &buf_;
    delete &trc_;
    delete &prevbinid_;
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


int SeisImporter::nrDone() const
{
    if ( postproc_ ) return postproc_->nrDone();
    return state_ == ReadBuf ? nrread_ : nrwritten_;
}


const char* SeisImporter::nrDoneText() const
{
    if ( postproc_ ) return postproc_->nrDoneText();
    return state_ == ReadBuf ? "Traces read" : "Traces written";
}


int SeisImporter::totalNr() const
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
	    return Executor::ErrorOccurred; \
	atend = true; \
    }


int SeisImporter::nextStep()
{
    if ( postproc_ ) return postproc_->doStep();

    if ( state_ == WriteBuf )
    {
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
	mDoRead( trc_ )
	if ( !atend )
	    return sortingOk(trc_) ? doWrite(trc_) : Executor::ErrorOccurred;

	postproc_ = mkPostProc();
	if ( !postproc_ && needInlCrlSwap() )
	{
	    errmsg_ = "Your data was loaded with swapped inline/crossline\n"
		      "Please use the batch program 'cbvs_swap_inlcrl' now\n";
	    return Executor::ErrorOccurred;
	}
	return postproc_ ? Executor::MoreToDo : Executor::Finished;
    }

    return readIntoBuf();
}


bool SeisImporter::needInlCrlSwap() const
{
    return !Seis::is2D(geomtype_) && sorting_ && !sorting_->inlSorted();
}


int SeisImporter::doWrite( SeisTrc& trc )
{
    if ( removenulltrcs_ && trc.isNull() )
	return Executor::MoreToDo;

    if ( needInlCrlSwap() )
	Swap( trc.info().binid.inl, trc.info().binid.crl );

    if ( wrr_.put(trc) )
    {
	nrwritten_++;
	return Executor::MoreToDo;
    }

    errmsg_ = wrr_.errMsg();
    if ( errmsg_.isEmpty() )
    {
	pErrMsg( "Need an error message from writer" );
	errmsg_ = "Cannot write trace";
    }

    return Executor::ErrorOccurred;
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
	    return Executor::ErrorOccurred;
	}

	state_ = buf_.isEmpty() ? ReadWrite : WriteBuf;
	return Executor::MoreToDo;
    }

    if ( Seis::is2D(geomtype_) || SI().isReasonable(trc->info().binid) )
    {
	buf_.add( trc );
	if ( !sortingOk(*trc) )
	    return Executor::ErrorOccurred;
	if ( !sortanal_ )
	    state_ = WriteBuf;
    }

    return Executor::MoreToDo;
}


bool SeisImporter::sortingOk( const SeisTrc& trc )
{
    const bool is2d = Seis::is2D(geomtype_);
    BinID bid( trc.info().binid );
    if ( is2d )
    {
	bid.crl = trc.info().nr;
	bid.inl = prevbinid_.inl;
	if ( trc.info().new_packet && !mIsUdf(prevbinid_.inl) )
	    bid.inl = prevbinid_.inl + 1;
    }

    bool rv = true;
    if ( sorting_ )
    {
	if ( !sorting_->isValid(prevbinid_,bid) )
	{
	    if ( is2d )
	    {
		errmsg_ = "Importing stopped at trace number: ";
		errmsg_ += trc.info().nr;
		errmsg_ += "because:\n";
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
	}
	else if ( *sortanal_->errMsg() )
	{
	    errmsg_ = sortanal_->errMsg();
	    rv = false;
	}
    }

    prevbinid_ = bid;
    return rv;
}


Executor* SeisImporter::mkPostProc()
{
    if ( needInlCrlSwap() )
    {
	pErrMsg( "TODO: inl/crl re-sorting Executor needed" );
	return 0;
    }
    return 0;
}
