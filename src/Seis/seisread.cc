/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-1-1998
 * FUNCTION : Seismic data reader
-*/

static const char* rcsID = "$Id: seisread.cc,v 1.22 2003-06-19 13:38:32 bert Exp $";

#include "seisread.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "executor.h"
#include "iostrm.h"
#include "survinfo.h"


SeisTrcReader::SeisTrcReader( const IOObj* ioob )
	: SeisStorage(ioob)
    	, outer(0)
{
    init();
}


void SeisTrcReader::init()
{
    foundvalidinl = foundvalidcrl =
    new_packet = needskip = started = forcefloats = false;
    prev_inl = -9999;
    delete outer; outer = 0;
}


class SeisReadStarter : public Executor
{
public:

const char* message() const
{ return tselst ? tselst->message() : msg; }
int nrDone() const
{ return tselst ? tselst->nrDone() : (rdr.curConn() ? 2 : 1); }
const char* nrDoneText() const
{ return tselst ? tselst->nrDoneText() : "Step"; }
int totalNr() const
{ return tselst ? tselst->totalNr() : 2; }

SeisReadStarter( SeisTrcReader& r )
	: Executor("Seismic Reader Starter")
	, rdr(r)
	, msg("Opening data store")
	, tselst(0)
	, conn(0)
{
    if ( rdr.trcsel )
	tselst = rdr.trcsel->starter();
}

~SeisReadStarter()
{
    delete tselst;
    delete conn;
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
	int rv = tselst->doStep();
	if ( rv < 0 ) return rv;
	if ( rv > 0 ) return rv;
	delete tselst; tselst = 0;
	return 1;
    }

    conn = rdr.openFirst();
    if ( !conn )
    {
	msg = "Cannot open seismic data";
	return -1;
    }
    else if ( !rdr.initRead(conn) )
    {
	msg = rdr.errMsg();
	return -1;
    }
    conn = 0;

    if ( rdr.forcefloats )
    {
	ObjectSet<SeisTrcTranslator::TargetComponentData>& tarcds =
					rdr.translator()->componentInfo();
	for ( int idx=0; idx<tarcds.size(); idx++ )
	    tarcds[idx]->datachar = DataCharacteristics();
    }

    rdr.started = true;
    rdr.outer = !rdr.trcsel || rdr.trcsel->isEmpty() || !rdr.trcsel->bidsel ? 0
	      : rdr.trcsel->bidsel->getOuter();
    return 0;
}

    Conn*		conn;
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


Conn* SeisTrcReader::openFirst()
{
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( iostrm )
	iostrm->setConnNr( iostrm->fileNumbers().start );

    trySkipConns();

    Conn* conn = ioobj->getConn( Conn::Read );
    if ( !conn || conn->bad() )
    {
	delete conn; conn = 0;
	if ( iostrm && multiConn() )
	{
	    while ( !conn || conn->bad() )
	    {
		delete conn; conn = 0;
		if ( !iostrm->toNextConnNr() ) break;

		conn = ioobj->getConn( Conn::Read );
	    }
	}
    }
    return conn;
}


bool SeisTrcReader::initRead( Conn* conn )
{
    if ( !trl->initRead(conn) )
    {
	errmsg = trl->errMsg();
	cleanUp();
	return false;
    }

    // Make sure the right component(s) are selected.
    // If all else fails, take component 0.
    const int nrcomp = trl->componentInfo().size();
    bool foundone = false;
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	if ( trl->componentInfo()[idx]->destidx >= 0 )
	    { foundone = true; break; }
    }
    if ( !foundone )
    {
	for ( int idx=0; idx<nrcomp; idx++ )
	{
	    if ( selcomp == -1 )
		trl->componentInfo()[idx]->destidx = idx;
	    else
		trl->componentInfo()[idx]->destidx = selcomp == idx ? 0 : 1;
	    if ( trl->componentInfo()[idx]->destidx >= 0 )
		foundone = true;
	}
	if ( !foundone )
	    trl->componentInfo()[0]->destidx = 0;
    }

    needskip = false;
    return true;
}


bool SeisTrcReader::doStart()
{
    Executor* st = starter();
    if ( !st->execute(0) )
    {
	errmsg = st->message();
	delete st;
	return false;
    }
    delete st; 
    return true;
}


int SeisTrcReader::get( SeisTrcInfo& ti )
{
    if ( !started && !doStart() )
	return -1;

    bool needsk = needskip; needskip = false;
    if ( needsk && !trl->skip() )
	return nextConn( ti );

    if ( !trl->readInfo(ti) )
	return nextConn( ti );

    ti.stack_count = 1;
    ti.new_packet = false;

    if ( prev_inl == -9999 )
	prev_inl = ti.binid.inl;
    else if ( prev_inl != ti.binid.inl )
    {
	foundvalidcrl = false;
	prev_inl = ti.binid.inl;
	ti.new_packet = true;
    }

    const int selres = !trcsel || !trcsel->bidsel ? 0
		     : trcsel->bidsel->excludes(ti.binid);
    if ( selres / 256 == 0 )
	foundvalidcrl = true;
    if ( selres % 256 == 0 )
	foundvalidinl = true;

    if ( selres )
    {
	if ( trl->inlCrlSorted() )
	{
	    int outerres = outer ? outer->excludes(ti.binid) : 0;
	    if ( foundvalidinl && outerres % 256 == 2 )
		return false;

	    bool neednewinl =  selres % 256 == 2
			    || foundvalidcrl && outerres / 256 == 2;
	    if ( neednewinl )
	    {
		mDynamicCastGet(IOStream*,iostrm,ioobj)
		if ( iostrm && iostrm->directNumberMultiConn() )
		    return nextConn(ti);
	    }
	}

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

    // This trace is, believe it or not, actually selected
    if ( new_packet )
    {
	ti.new_packet = true;
	new_packet = false;
    }
    needskip = true;
    return 1;
}


bool SeisTrcReader::get( SeisTrc& trc )
{
    needskip = false;
    if ( !started && !doStart() )
	return false;

    if ( !trl->read(trc) )
    {
	errmsg = trl->errMsg();
	trl->skip();
	return false;
    }
    return true;
}


int SeisTrcReader::nextConn( SeisTrcInfo& ti )
{
    new_packet = false;
    if ( !multiConn() ) return 0;

    trl->cleanUp();
    IOStream* iostrm = (IOStream*)ioobj;
    if ( !iostrm->toNextConnNr() )
	return 0;

    trySkipConns();
    Conn* conn = iostrm->getConn( Conn::Read );

    while ( !conn || conn->bad() )
    {
	delete conn; conn = 0;
	if ( !iostrm->toNextConnNr() ) return 0;

	trySkipConns();
	conn = iostrm->getConn( Conn::Read );
    }

    if ( !trl->initRead(conn) )
    {
	errmsg = trl->errMsg();
	return -1;
    }
    int rv = get(ti);
    if ( rv < 1 ) return rv;

    if ( trcsel && trcsel->bidsel && iostrm->directNumberMultiConn() )
    {
	if ( !binidInConn(trcsel->bidsel->excludes(ti.binid)) )
	    return nextConn( ti );
    }

    if ( rv == 2 )	new_packet = true;
    else		ti.new_packet = true;

    return rv;
}


void SeisTrcReader::trySkipConns()
{
    if ( !multiConn() || !trcsel || !trcsel->bidsel ) return;
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( !iostrm || !iostrm->directNumberMultiConn() ) return;

    BinID binid;
    const BinIDSelector* sel = trcsel->bidsel;
    BinIDProvider* bp = sel->provider();
    if ( bp && bp->size() == 0 ) { delete bp; return; }

    if ( !bp )	binid = SI().range().start;
    else	binid = (*bp)[0];
    delete bp;

    do
    {
	binid.inl = iostrm->connNr();
	if ( binidInConn(sel->excludes(binid)) )
	    return;

    } while ( iostrm->toNextConnNr() );
}


bool SeisTrcReader::binidInConn( int r ) const
{
    return r == 0
	|| !trl->inlCrlSorted()
	|| r%256 != 2;
}


void SeisTrcReader::fillPar( IOPar& iopar ) const
{
    SeisStorage::fillPar( iopar );
    if ( trl && trl->trcSel() ) trl->trcSel()->fillPar( iopar );
}
