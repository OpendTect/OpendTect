/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-1-1998
 * FUNCTION : Seismic data reader
-*/

static const char* rcsID = "$Id: seisread.cc,v 1.9 2001-02-13 17:21:08 bert Exp $";

#include "seisread.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "storlayout.h"
#include "executor.h"
#include "iostrm.h"
#include "survinfo.h"


SeisTrcReader::SeisTrcReader( const IOObj* ioob )
	: SeisStorage(ioob)
{
    init();
}


void SeisTrcReader::init()
{
    icfound = new_packet = needskip = rdinited = false;
}


class SeisReadStarter : public Executor
{
public:

const char* message() const
{ return tselst ? tselst->message() : msg; }
int nrDone() const
{ return tselst ? tselst->nrDone() : (rdr.conn ? 2 : 1); }
const char* nrDoneText() const
{ return tselst ? tselst->nrDoneText() : "Step"; }
int totalNr() const
{ return tselst ? tselst->totalNr() : 2; }

SeisReadStarter( SeisTrcReader& r )
	: Executor("Seismic Reader Starter")
	, rdr(r)
	, msg("Opening data store")
	, tselst(0)
{
    if ( rdr.trcsel )
	tselst = rdr.trcsel->starter();
}

~SeisReadStarter()
{
    delete tselst;
}


int nextStep()
{
    if ( !rdr.ioObj() )
    {
	msg = "No info from Object Manager";
	return -1;
    }

    if ( tselst )
    {
	int rv = tselst->nextStep();
	if ( rv < 0 ) return rv;
	if ( rv > 0 ) return rv;
	delete tselst; tselst = 0;
	return 1;
    }

    if ( rdr.connState() != Conn::Read )
    {
	if ( !rdr.openFirst() )
	{
	    msg = "Cannot open seismic data";
	    return -1;
	}
	msg = "Reading global header data";
	return YES;
    }

    if ( !rdr.initRead() )
    {
	msg = rdr.errMsg();
	return -1;
    }

    return NO;
}

    SeisTrcReader&	rdr;
    const char*		msg;
    Executor*		tselst;

};


Executor* SeisTrcReader::starter()
{
    return new SeisReadStarter( *this );
}


bool SeisTrcReader::multiConn() const
{
    return ioobj && ioobj->hasClass(IOStream::classid)
	&& ((IOStream*)ioobj)->multiConn();
}


bool SeisTrcReader::openFirst()
{
    trySkipConns();
    conn = ioobj->getConn( Conn::Read );
    if ( !conn || conn->bad() )
    {
	delete conn; conn = 0;
	if ( multiConn() )
	{
	    while ( !conn || conn->bad() )
	    {
		delete conn; conn = 0;
		if ( !((IOStream*)ioobj)->toNextConnNr() ) break;
		conn = ioobj->getConn( Conn::Read );
	    }
	}
    }
    return conn ? true : false;
}


bool SeisTrcReader::initRead()
{
    if ( !trl->initRead(*conn) )
    {
	errmsg = trl->errMsg();
	cleanUp();
	return false;
    }

    needskip = false;
    rdinited = true;
    return true;
}


int SeisTrcReader::get( SeisTrcInfo& ti )
{
    if ( !rdinited )
    {
	Executor* st = starter();
	if ( !st->execute(0) )
	{
	    errmsg = st->message();
	    delete st;
	    return -1;
	}
	delete st; 
    }

    bool needsk = needskip; needskip = false;
    if ( needsk )
    {
	if ( !trl->skip() )
	{
	    if ( trl->errMsg() && *trl->errMsg() )
		{ errmsg = trl->errMsg(); return -1; }
	    return nextConn( ti );
	}
    }

    if ( !trl->readInfo(ti) )
    {
	if ( trl->errMsg() && *trl->errMsg() )
	    { errmsg = trl->errMsg(); return -1; }
	return nextConn( ti );
    }
    ti.stack_count = 1;
    ti.new_packet = false;

    const BinIDSelector* sel = trcsel ? trcsel->bidsel : 0;
    int res = 0;
    if ( sel && (res = sel->excludes(ti.binid)) )
    {
	// Now, we must skip the trace, but ...
	const StorageLayout& lyo = trl->storageLayout();
	bool clustinl = lyo.type() == StorageLayout::Inline;
	bool clustcrl = lyo.type() == StorageLayout::Xline;
	bool isrange = sel->type() == 0;
	if ( clustinl || clustcrl )
	{
	    if ( lyo.isCont() )
	    {
		// ... maybe we can stop now
		if ( isrange && icfound && !validBidselRes(res,clustcrl) )
		    return 0;
	    }
	    else
	    {
		// ... maybe we can go to the next file
		if (
		     ( isrange && icfound )
		  || ( ((clustinl && res%256==2) || (clustcrl && res/256==2)) )
		   )
		{
		    icfound = false;
		    return nextConn( ti );
		}
		icfound = false;
	    }
	}

	// ... no, we must skip
	return trl->skip() ? 2 : nextConn( ti );
    }

    nrtrcs++;
    if ( trcsel )
    {
	if ( nrtrcs < trcsel->intv.start
	  ||    (trcsel->intv.step > 1
	     && (nrtrcs-trcsel->intv.start-1)%trcsel->intv.step) )
	    return trl->skip() ? 2 : nextConn( ti );
	else if ( nrtrcs > trcsel->intv.stop )
	    return 0;
    }

    // This trace is actually selected
    if ( new_packet )
    {
	ti.new_packet = true;
	new_packet = false;
    }
    icfound = needskip = true;
    return 1;
}


bool SeisTrcReader::get( SeisTrc& trc )
{
    needskip = false;
    if ( !trl->read(trc) )
    {
	errmsg = trl->errMsg();
	return false;
    }
    return true;
}


int SeisTrcReader::nextConn( SeisTrcInfo& ti )
{
    new_packet = false;
    if ( !multiConn() ) return 0;
    IOStream* iostrm = (IOStream*)ioobj;

    delete conn; conn = 0;
    if ( !iostrm->toNextConnNr() ) return 0;

    trySkipConns();
    conn = iostrm->getConn( Conn::Read );

    while ( !conn || conn->bad() )
    {
	delete conn; conn = 0;
	if ( !iostrm->toNextConnNr() ) return 0;

	trySkipConns();
	conn = iostrm->getConn( Conn::Read );
    }

    icfound = false;

    if ( !trl->initRead(*conn) )
    {
	errmsg = trl->errMsg();
	return -1;
    }
    int rv = get(ti);
    if ( rv < 1 ) return rv;

    if ( trcsel && trcsel->bidsel && iostrm->directNumberMultiConn() )
    {
	if ( !validBidselRes( trcsel->bidsel->excludes(ti.binid),
		      trl->storageLayout().type() == StorageLayout::Xline) )
	    return nextConn( ti );
    }

    if ( rv == 2 )	new_packet = true;
    else		ti.new_packet = true;

    return rv;
}


void SeisTrcReader::trySkipConns()
{
    if ( !multiConn() || !trcsel || !trcsel->bidsel ) return;
    const BinIDSelector* sel = trcsel->bidsel;
    IOStream* iostrm = (IOStream*)ioobj;
    if ( !iostrm->directNumberMultiConn() ) return;

    bool clustcrl = trl->storageLayout().type() == StorageLayout::Xline;
    BinID binid;
    BinIDProvider* bp = sel->provider();
    if ( bp && bp->size() == 0 ) { delete bp; return; }

    if ( !bp )	binid = SI().range().start;
    else	binid = (*bp)[0];
    int& newinlcrl = clustcrl ? binid.crl : binid.inl;
    do
    {
	newinlcrl = iostrm->connNr();
	if ( validBidselRes( sel->excludes(binid), clustcrl ) )
	    return;

    } while ( iostrm->toNextConnNr() );
}


void SeisTrcReader::fillPar( IOPar& iopar ) const
{
    SeisStorage::fillPar( iopar );
    if ( trl && trl->trcSel() ) trl->trcSel()->fillPar( iopar );
}
