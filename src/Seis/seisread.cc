/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-1-1998
 * FUNCTION : Seismic data reader
-*/

static const char* rcsID = "$Id: seisread.cc,v 1.6 2000-03-22 13:39:15 bert Exp $";

#include "seisread.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seiskeys.h"
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
    itrc = 0;
    icfound = NO;
    new_packet = NO;
    needskip = NO;
    connrisbidnr = NO;
}


class SeisReadStarter : public Executor
{
public:

const char* message() const
{ return keycr ? keycr->message() : msg; }
int nrDone() const
{ return keycr ? keycr->nrDone() : (rdr.conn ? 2 : 1); }
const char* nrDoneText() const
{ return keycr ? keycr->nrDoneText() : "Step"; }
int totalNr() const
{ return keycr ? keycr->totalNr() : 2; }

SeisReadStarter( SeisTrcReader& r )
	: Executor("Seismic Reader Starter")
	, rdr(r)
	, msg("Opening data store")
	, keycr(((SeisKeyData&)rdr.keyData()).starter())
{
}

~SeisReadStarter()
{
    delete keycr;
}


int nextStep()
{
    if ( !rdr.ioObj() )
    {
	msg = "Bad data from Object Manager";
	return -1;
    }

    if ( keycr )
    {
	int rv = keycr->nextStep();
	if ( rv < 0 ) return rv;
	if ( rv > 0 ) return rv;
	delete keycr; keycr = 0;
	return 1;
    }

    rdr.connrisbidnr = rdr.ioObj()->isStarConn();
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
    Executor*		keycr;

};


Executor* SeisTrcReader::starter()
{
    return new SeisReadStarter( *this );
}


bool SeisTrcReader::openFirst()
{
    trySkipConns();
    conn = ioobj->getConn( Conn::Read );
    if ( !conn || conn->bad() )
    {
	delete conn; conn = 0;
	if ( ioobj->multiConn() )
	{
	    while ( !conn || conn->bad() )
	    {
		delete conn; conn = 0;
		if ( !ioobj->toNextConnNr() ) break;
		conn = ioobj->getConn( Conn::Read );
	    }
	}

	if ( !conn ) { delete ioobj; ioobj = 0; }

    }
    return conn ? YES : NO;
}


bool SeisTrcReader::initRead()
{
    if ( !trl->initRead(spi,*conn) )
    {
	delete ioobj; ioobj = 0;
	delete conn; conn = 0;
	errmsg = trl->errMsg();
	return NO;
    }

    needskip = NO;
    return YES;
}


bool SeisTrcReader::prepareRetry()
{
    if ( trl ) trl->prepareRetry();
    return true;
}


const SeisKeyData& SeisTrcReader::keyData() const
{
    static SeisKeyData dum_skd;
    return trl ? trl->keyData() : dum_skd;
}


void SeisTrcReader::setKeyData( const SeisKeyData& skd )
{
    if ( trl ) trl->keyData() = skd;
}


int SeisTrcReader::get( SeisTrcInfo& ti )
{
    bool needsk = needskip; needskip = NO;
    if ( needsk )
    {
	if ( !trl->skip() )
	{
	    if ( trl->errMsg() )
		{ errmsg = trl->errMsg(); return -1; }
	    return nextConn( ti );
	}
    }
    if ( !trl->read(ti) )
    {
	if ( trl->errMsg() )
	    { errmsg = trl->errMsg(); return -1; }
	return nextConn( ti );
    }
    finishGetInfo( ti );

    const SeisKeyData& skd = keyData();
    BinIDSelector* sel = skd.bidsel;
    int res = 0;
    if ( sel && (res = sel->excludes(ti.binid)) )
    {
	// Now, we must skip the trace, but ...
	const StorageLayout& lyo = storageLayout();
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
		    icfound = NO;
		    return nextConn( ti );
		}
		icfound = NO;
	    }
	}

	// ... no, we must skip
	return trl->skip() ? 2 : nextConn( ti );
    }

    itrc++;
    if ( itrc-1 < skd.intv.start
      || (skd.intv.step > 1 && (itrc-skd.intv.start-1)%skd.intv.step) )
	return trl->skip() ? 2 : nextConn( ti );
    else if ( itrc-1 > skd.intv.stop )
	return 0;

    // This trace is actually selected
    if ( new_packet )
    {
	ti.new_packet = YES;
	new_packet = NO;
    }
    icfound = needskip = YES;
    return 1;
}


bool SeisTrcReader::get( SeisTrc& trc )
{
    needskip = NO;
    if ( !trl->read(trc) )
    {
	errmsg = trl->errMsg();
	trl->clearErr();
	return NO;
    }
    return YES;
}


int SeisTrcReader::nextConn( SeisTrcInfo& ti )
{
    new_packet = NO;
    if ( !ioobj->multiConn()
      || (keyData().bidsel && keyData().bidsel->size() == 0) )
	return 0;

    delete conn; conn = 0;
    if ( !ioobj->toNextConnNr() ) return 0;

    trySkipConns();
    conn = ioobj->getConn( Conn::Read );

    while ( !conn || conn->bad() )
    {
	delete conn; conn = 0;
	if ( !ioobj->toNextConnNr() ) return 0;

	trySkipConns();
	conn = ioobj->getConn( Conn::Read );
    }

    icfound = NO;

    if ( !trl->initRead(spi,*conn) )
    {
	errmsg = trl->errMsg();
	return -1;
    }
    int rv = get(ti);
    if ( rv < 1 ) return rv;

    if ( keyData().bidsel && connrisbidnr )
    {
	if ( !validBidselRes(keyData().bidsel->excludes(ti.binid),
			     storageLayout().type() == StorageLayout::Xline) )
	    return nextConn( ti );
    }

    if ( rv == 2 )	new_packet = YES;
    else		ti.new_packet = YES;

    return rv;
}


void SeisTrcReader::trySkipConns()
{
    const SeisKeyData& skd = keyData();
    if ( !ioobj->multiConn() || !skd.bidsel || skd.bidsel->size() == 0
      || !connrisbidnr )
	return;

    bool clustcrl = storageLayout().type() == StorageLayout::Xline;
    BinID binid = (*skd.bidsel)[0];
    int& newinlcrl = clustcrl ? binid.crl : binid.inl;
    do
    {
	newinlcrl = ioobj->connNr();
	if ( validBidselRes(skd.bidsel->excludes(binid),clustcrl) )
	    return;

    } while ( ioobj->toNextConnNr() );
}


void SeisTrcReader::finishGetInfo( SeisTrcInfo& ti )
{
    BinID& binid = ti.binid;
    if ( binid.inl == 0 && binid.crl == 0 )
	binid = SI().transform( ti.coord );
    else if ( !trl->validCoords() )
	ti.coord = SI().transform( binid );

    ti.stack_count = 1;
    ti.new_packet = NO;
    ti.starttime += ti.dt * 1e-6 * trl->keyData().sg.start;
    ti.dt *= trl->step();
}


void SeisTrcReader::usePar( const IOPar& iopar )
{
    SeisStorage::usePar( iopar );
    if ( trl ) trl->keyData().usePar( iopar );
}

 
void SeisTrcReader::fillPar( IOPar& iopar ) const
{
    SeisStorage::fillPar( iopar );
    if ( trl ) trl->keyData().fillPar( iopar );
}
