/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2006
-*/

static const char* rcsID = "$Id: seisimporter.cc,v 1.2 2006-12-05 15:21:41 cvsbert Exp $";

#include "seisimporter.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "ptrman.h"


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
    	, crlsorted_(0)
    	, postproc_(0)
    	, removenulltrcs_(false)
{
}


SeisImporter::~SeisImporter()
{
    buf_.deepErase();
    delete postproc_;
    delete rdr_;
    delete &buf_;
    delete &trc_;
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
	{
	    if ( crlsorted_ )
		Swap( trc_.info().binid.inl, trc_.info().binid.crl );

	    return doWrite( trc_ );
	}

	postproc_ = mkPostProc();
	return postproc_ ? Executor::MoreToDo : Executor::Finished;
    }

    return readIntoBuf();
}


int SeisImporter::doWrite( const SeisTrc& trc )
{
    if ( removenulltrcs_ && trc.isNull() )
	return Executor::MoreToDo;

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

    buf_.add( trc );

    //TODO use the info we have to make importing safer
    state_ = WriteBuf;
    return Executor::MoreToDo;
}


Executor* SeisImporter::mkPostProc()
{
    if ( crlsorted_ )
	return 0; //TODO inl/crl re-sorting Executor needed
    return 0;
}
