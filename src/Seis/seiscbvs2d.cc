/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/

static const char* rcsID = "$Id: seiscbvs2d.cc,v 1.2 2004-07-19 11:30:10 bert Exp $";

#include "seiscbvs2d.h"
#include "seiscbvs.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "cbvsio.h"
#include "executor.h"
#include "survinfo.h"
#include "keystrs.h"
#include "filegen.h"
#include "iopar.h"
#include "errh.h"

#define mGetFname(iop,lnr) \
	CBVSIOMgr::getFileName( iop.find(sKey::FileName), lnr )

int SeisCBVS2DLineIOProvider::factid
	= (S2DLIOPs() += new SeisCBVS2DLineIOProvider).size() - 1;


static int getLineNr( const IOPar* iop, int lnr=0 )
{
    if ( iop )
	iop->get( Seis2DLineIOProvider::sKeyLineNr, lnr );
    return lnr;
}


SeisCBVS2DLineIOProvider::SeisCBVS2DLineIOProvider()
    	: Seis2DLineIOProvider("CBVS")
{
}


bool SeisCBVS2DLineIOProvider::isUsable( const IOPar& iop ) const
{
    return Seis2DLineIOProvider::isUsable(iop) && iop.find( sKeyLineNr );
}


bool SeisCBVS2DLineIOProvider::isEmpty( const IOPar& iop ) const
{
    if ( !isUsable(iop) ) return true;

    BufferString fnm = mGetFname( iop, getLineNr(&iop) );
    return File_isEmpty(fnm);
}


#undef mErrRet
#define mErrRet(s) { msg = s; return -1; }

class SeisCBVS2DLineGetter : public Executor
{
public:

SeisCBVS2DLineGetter( const char* fnm, SeisTrcBuf& b )
    	: Executor("Load 2D line")
	, tbuf(b)
	, curnr(0)
	, totnr(0)
	, fname(fnm)
	, tr(CBVSSeisTrcTranslator::getInstance())
	, msg("Reading traces")
{
}


~SeisCBVS2DLineGetter()
{
    delete tr;
}


void addTrc( SeisTrc* trc )
{
    trc->info().binid = SI().transform( trc->info().coord );
    tbuf.add( trc );
}


int nextStep()
{
    if ( curnr == 0 )
    {
	if ( !tr->initRead(new StreamConn(fname.buf(),Conn::Read)) )
	    mErrRet(tr->errMsg())
	const SeisPacketInfo& pinf = tr->packetInfo();
	totnr = tr->packetInfo().crlrg.nrSteps() + 1;
    }

    int lastnr = curnr + 10;
    for ( ; curnr<lastnr; curnr++ )
    {
	SeisTrc* trc = new SeisTrc;
	if ( !tr->read(*trc) )
	{
	    delete trc;
	    if ( tbuf.size() < 1 )
		mErrRet("No traces read in 2D line")
	    return 0;
	}
	addTrc( trc );
    }

    return 1;
}

const char*		message() const		{ return msg; }
const char*		nrDoneText() const	{ return "Traces read"; }
int			nrDone() const		{ return curnr; }
int			totalNr() const		{ return totnr; }

    int			curnr;
    int			totnr;
    SeisTrcBuf&		tbuf;
    BufferString	fname;
    BufferString	msg;
    CBVSSeisTrcTranslator* tr;

};


#undef mErrRet
#define mErrRet(s) { errmsg = s; return 0; }

Executor* SeisCBVS2DLineIOProvider::getFetcher( const IOPar& iop,
						SeisTrcBuf& tbuf )
{
    if ( !isUsable(iop) )
    {
	BufferString fnm = mGetFname( iop, getLineNr(&iop) );
	BufferString errmsg = "2D seismic line file '"; errmsg += fnm;
	errmsg += "' does not exist";
	ErrMsg(errmsg);
	return 0;
    }

    const int lnr = getLineNr( &iop );
    BufferString fnm = mGetFname( iop, lnr );
    const_cast<IOPar&>(iop).set( sKeyLineNr, lnr ); // just to be sure
    return new SeisCBVS2DLineGetter( fnm, tbuf );
}


#undef mErrRet
#define mErrRet(s) { msg = s; return -1; }

class SeisCBVS2DLinePutter : public Executor
{
public:

SeisCBVS2DLinePutter( const char* fnm, const SeisTrcBuf& b, int linenr )
    	: Executor("Store 2D line")
	, tbuf(b)
	, curnr(0)
	, fname(fnm)
	, tr(CBVSSeisTrcTranslator::getInstance())
	, crlrg(SI().crlRange(false))
	, msg("Writing traces")
{
    bid.inl = SI().inlRange(false).start + linenr * SI().inlRange(false).step;
}


~SeisCBVS2DLinePutter()
{
    delete tr;
}


void updTrc()
{
    if ( curnr >= tbuf.size() ) return;
    bid.crl = crlrg.start + curnr * crlrg.step;
    trc = const_cast<SeisTrc*>( tbuf.get( curnr ) );
    oldbid = trc->info().binid;
    trc->info().binid = bid;
}

int nextStep()
{
    if ( curnr == 0 )
    {
	if ( tbuf.size() <= curnr )
	    mErrRet("No traces in 2D line")
	updTrc();
	bool res = tr->initWrite(new StreamConn(fname.buf(),Conn::Write),*trc);
	trc->info().binid = oldbid;
	if ( !res )
	    mErrRet("Cannot open the output file for 2D line")
    }

    int lastnr = curnr + 10;
    if ( lastnr >= tbuf.size() ) lastnr = tbuf.size() - 1;
    for ( ; curnr<lastnr; curnr++ )
    {
	updTrc();
	bool res = tr->write(*trc);
	trc->info().binid = oldbid;
	if ( !res )
	    mErrRet("Error during trace write to 2D line")
    }

    return curnr >= tbuf.size() ? 0 : 1;
}

const char*		message() const		{ return msg; }
const char*		nrDoneText() const	{ return "Traces written"; }
int			nrDone() const		{ return curnr; }
int			totalNr() const		{ return tbuf.size(); }

    int			curnr;
    int			inl;
    StepInterval<int>	crlrg;
    const SeisTrcBuf&	tbuf;
    BufferString	fname;
    BufferString	msg;
    CBVSSeisTrcTranslator* tr;
    SeisTrc*		trc;
    BinID		bid;
    BinID		oldbid;

};


#undef mErrRet
#define mErrRet(s) { ErrMsg( s ); return 0; }

Executor* SeisCBVS2DLineIOProvider::getPutter( IOPar& iop,
					       const SeisTrcBuf& tbuf,
	                                       const IOPar* previop )
{
    if ( !isUsable(iop) ) return 0;
    if ( tbuf.size() < 1 )
	mErrRet("No traces to write")

    const int lnr = getLineNr(previop,-1) + 1;
    BufferString fnm = mGetFname( iop, lnr );
    iop.set( sKeyLineNr, lnr );
    return new SeisCBVS2DLinePutter( fnm, tbuf, lnr );
}
