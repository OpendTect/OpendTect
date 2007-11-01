/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R. K. Singh
 * DATE     : Oct 2007
-*/

static const char* rcsID = "$Id: seispsmerge.cc,v 1.1 2007-11-01 07:08:25 cvsraman Exp $";

#include "seispsmerge.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seiscbvsps.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "cubesampling.h"
#include "dirlist.h"
#include "filegen.h"
#include "multiid.h"
#include "survinfo.h"
#include "ioobj.h"
#include "iopar.h"
#include "ioman.h"
#include "ptrman.h"


int getInlFromFileNm( BufferString fnm )
{
    char* ptr = fnm.buf();
    while ( *ptr && !isdigit(*ptr) ) ptr++;
    while ( *ptr && isdigit(*ptr) ) ptr++;
    *ptr = '\0';
    if ( fnm.isEmpty() ) return -1;

    return atoi( fnm.buf() );
}


SeisPSMerger::SeisPSMerger( ObjectSet<IOObj> objset, const IOObj* out ) 
  	: Executor("Pre-Stack data Merger")
	, inobjs_(objset)
	, outobj_(out)
	, curinlidx_(-1)
	, writer_(0)
	, msg_("Nothing")
	, totnr_(-1)
	, nrdone_(0)
{
    if ( inobjs_.isEmpty() )
    {
	msg_ = "No input specified";
	return;
    }

    nrobjs_ = inobjs_.size();
    init();
}


#define mDeleteNull(ptr) \
    if ( ptr ) \
    { delete ptr; ptr = 0; }

SeisPSMerger::~SeisPSMerger()
{
    mDeleteNull(writer_);
    deepErase( pinfoset_ );
    deepErase( readers_ );
}


bool SeisPSMerger::init()
{
    totnr_ = 0;
    inlset_.erase();
    lineobjlist_.erase();
    pinfoset_.erase();
    for ( int idx=0; idx<nrobjs_; idx++ )
    {
	DirList inlist( inobjs_[idx]->fullUserExpr(true), DirList::FilesOnly );
	for ( int fdx=0; fdx<inlist.size(); fdx++ )
	{
	    const int line = getInlFromFileNm( inlist.get(fdx) );
	    int lidx = inlset_.indexOf( line );
	    if ( lidx < 0 )
	    { inlset_ += line; lidx = inlset_.size() - 1; }

	    CBVSSeisTrcTranslator* tr =
			CBVSSeisTrcTranslator::make( inlist.fullPath(fdx),
		    				     true, false, &msg_ );
	    StreamConn* scon = new StreamConn( inlist.fullPath(fdx),
		    			       Conn::Read );
	    tr->initRead( scon, Seis::PreScan );
	    SeisPacketInfo spi = tr->packetInfo();
	    if ( lidx < pinfoset_.size() )
	    {
		SeisPacketInfo& prevspi = *pinfoset_[lidx];
		if ( prevspi.inlrg.step != spi.inlrg.step ||
		     prevspi.crlrg.step != spi.crlrg.step ||
		     prevspi.zrg != spi.zrg )
		{
		    pErrMsg( "Data Mismatch" );
		    continue;
		}

		prevspi.inlrg.include( spi.inlrg );
		lineobjlist_[lidx] += idx;
	    }
	    else
	    {
		pinfoset_ += new SeisPacketInfo( spi );
		TypeSet<int> objlist;
		objlist += idx;
		lineobjlist_ += objlist;
	    }
	    delete tr;
	}
    }

    for ( int lidx=0; lidx<pinfoset_.size(); lidx++ )
	totnr_ += pinfoset_[lidx]->inlrg.nrSteps();

    curinlidx_ = 0;
    prepareReaders();

    writer_ = new SeisCBVSPSWriter( outobj_->fullUserExpr(false) );
    writer_->usePar( outobj_->pars() );
    return true;
}


int SeisPSMerger::prepareReaders()
{
    if ( curinlidx_ >= inlset_.size() )
	return 0;

    if ( curinlidx_>=lineobjlist_.size() || curinlidx_>=pinfoset_.size() )
	return -1;

    int inl = inlset_[curinlidx_];
    const TypeSet<int>& objids = lineobjlist_[curinlidx_];
    if ( objids.size() == 1 )
    {
	const int objidx = objids[0];
	BufferString from( inobjs_[objidx]->fullUserExpr(true) );
	BufferString fnm( inl );
	fnm += ".cbvs";
	BufferString to( outobj_->fullUserExpr(false) );
	if ( !File_isDirectory(to) )
	    File_createDir( to, 0 );

	from += "/"; from += fnm;
	to += "/"; to += fnm;
	File_copy( from, to, 0 );
	curinlidx_++;
	return prepareReaders();
    }

    deepErase( readers_ );
    for ( int idx=0; idx<objids.size(); idx++ )
    {
	const int objidx = objids[idx];
	BufferString dirnm( inobjs_[objidx]->fullUserExpr(true) );
	SeisCBVSPSReader* reader = new SeisCBVSPSReader( dirnm, inl );
	if ( !reader )
	{
	    pErrMsg( "Couldn't create translator" );
	    return -1;
	}

	readers_ += reader;
    }

    curbid_.inl = inl; curbid_.crl = -1;
    return 1;
}


const char* SeisPSMerger::message() const
{
    const char* msg = msg_.buf();
    return msg;
}


const char* SeisPSMerger::nrDoneText() const
{
    return "Traces written";
}


int SeisPSMerger::nrDone() const
{
    return nrdone_;
}


int SeisPSMerger::totalNr() const
{
    return totnr_;
}


int SeisPSMerger::nextStep()
{
    const int ret = doNextPos();
    if ( ret == 0 ) return Executor::Finished;
    if ( ret == -1 ) return Executor::ErrorOccurred;

    nrdone_ ++; curbid_.crl++;
    return Executor::MoreToDo;
}


int SeisPSMerger::doNextPos()
{
    SeisPacketInfo& spi = *pinfoset_[curinlidx_];
    if ( curbid_.crl > spi.inlrg.stop )
    {
	curinlidx_++;
	const int ret = prepareReaders();
	if ( ret != 1 ) return ret;
    }
    
    if ( curbid_.crl < 0 )
	curbid_.crl = spi.inlrg.start;

    const TypeSet<int>& objlist = lineobjlist_[curinlidx_];
    for ( int idx=0; idx<readers_.size(); idx++ )
    {
	SeisTrcBuf* trcbuf = new SeisTrcBuf(true);
	SeisCBVSPSReader* rdr = readers_[idx];
	if ( !rdr->getGather(curbid_, *trcbuf) )
	{
	    delete trcbuf;
	    continue;
	}

	for ( int tdx=0; tdx<trcbuf->size(); tdx++ )
	{
	    SeisTrc* trc = trcbuf->get( tdx );
	    if ( !writer_->put(*trc) )
		return -1;
	}

	delete trcbuf;
	break;
    }

    return 1;
}

