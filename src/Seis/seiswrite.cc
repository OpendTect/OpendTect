/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-1-1998
 * FUNCTION : Seismic data reader
-*/

static const char* rcsID = "$Id: seiswrite.cc,v 1.5 2000-11-09 15:54:03 bert Exp $";

#include "seiswrite.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "executor.h"
#include "ioobj.h"
#include "separstr.h"
#include "storlayout.h"
#include "binidsel.h"
#include "iopar.h"


SeisTrcWriter::SeisTrcWriter( const IOObj* ioob )
	: SeisStorage(ioob)
	, binids(0)
{
    init();
}


void SeisTrcWriter::init()
{
    delete binids; binids = new BinIDRange;
    starttime = 0;
    dt = 4000;
    nrwr = nrwrconn = 0;
    trace_size = -1;
}


class SeisWriteStarter : public Executor
{
public:

const char* message() const	{ return msg; }
int nrDone() const		{ return wr.conn ? 2 : 1; }
const char* nrDoneText() const	{ return "Step"; }

SeisWriteStarter( SeisTrcWriter& w )
	: Executor("Seismic Writer Starter")
	, wr(w)
	, msg("Initializing data store")
{
}


int nextStep()
{
    if ( !wr.ioObj() )
    {
	msg = "Bad data from Object Manager";
	return -1;
    }

    if ( wr.connState() != Conn::Write )
    {
	if ( !wr.openConn() )
	{
	    msg = "Cannot open data storage for write";
	    return -1;
	}
	msg = "Writing global header data";
	return 1;
    }

    if ( !wr.initWrite() )
    {
	msg = wr.errMsg();
	return -1;
    }

    return 0;
}

    SeisTrcWriter&	wr;
    const char*		msg;

};


Executor* SeisTrcWriter::starter()
{
    return new SeisWriteStarter( *this );
}


bool SeisTrcWriter::openConn()
{
    if ( !ioobj ) return false;

    delete conn;
    conn = ioobj->getConn( Conn::Write );
    if ( !conn || conn->bad() )
    {
	delete ioobj; ioobj = 0;
	delete conn; conn = 0;
    }

    return conn ? true : false;
}


bool SeisTrcWriter::initWrite()
{
    if ( trl && trl->initWrite(spi,*conn) )
	return true;

    delete ioobj; ioobj = 0;
    delete conn; conn = 0;
    errmsg = trl ? trl->errMsg() : "Error initialising data store";
    return false;
}


bool SeisTrcWriter::handleConn( const SeisTrcInfo& ti )
{
    if ( !ioobj->multiConn() ) return true;

    bool neednewpacket = ti.new_packet;
    const StorageLayout& lyo = storageLayout();
    bool clustcrl = lyo.type() == StorageLayout::Xline;

    if ( !neednewpacket && ioobj->isStarConn() )
	neednewpacket = ioobj->connNr() != (lyo.type() == StorageLayout::Xline
					    ? ti.binid.crl : ti.binid.inl);

    if ( !neednewpacket ) return true;

    delete conn; conn = 0;
    if ( nrwrconn == 0 )
	(void)ioobj->implRemove(); // What should we do if it fails? Can't stop.
    nrwrconn = 0;
    if ( !ioobj->toNextConnNr() ) return false;

    if ( ioobj->isStarConn() )
    {
	int nr = clustcrl ? ti.binid.crl : ti.binid.inl;
	while ( nr != ioobj->connNr() && ioobj->toNextConnNr() )
	    ;
	if ( nr != ioobj->connNr() )
	{
	    errmsg = "Output line "; errmsg += nr;
	    errmsg += " is not in valid range for output";
	    return false;
	}
    }

    if ( !openConn() )
    {
	if ( ioobj && errmsg == "" ) errmsg = "Cannot create file";
	return false;
    }

    return initWrite();
}


bool SeisTrcWriter::prepareRetry()
{
    if ( trl ) trl->prepareRetry();
    return true;
}


bool SeisTrcWriter::put( const SeisTrc& t )
{
    if ( !trl ) { errmsg = "No translator"; return false; }
    if ( !handleConn( t.info() ) ) return false;

    const SeisTrc* trc = &t;
    SeisTrc* made_trc = 0;
    if ( trace_size <  0 )
    {
	trace_size = trc->size();
	starttime = trc->info().starttime;
	dt = trc->info().dt;
	binids->start = trc->info().binid;
	binids->stop = trc->info().binid;
    }
    else
    {
	binids->include( trc->info().binid );
	if ( trc->size() != trace_size )
	{
	    made_trc = trc->getNew();
	    made_trc->info() = trc->info();
	    made_trc->reSize( trace_size );
	    int copy_size = trace_size;
	    if ( trace_size > trc->size() )
	    {
		made_trc->clear();
		copy_size = trc->size();
	    }
	    for ( int idx=0; idx<copy_size; idx++ )
		made_trc->set( idx, (*trc)[idx] );
	    trc = made_trc;
	}
    }

    bool rv = true;
    if ( trl->write(*trc) )
	{ nrwrconn++; nrwr++; }
    else
    {
	errmsg = trl->errMsg();
	delete trl; trl = 0;
	rv = false;
    }

    delete made_trc;
    return rv;
}


void SeisTrcWriter::fillAuxPar( IOPar& iopar ) const
{
    if ( nrwr < 1 ) return;

    FileMultiString fms;
    fms += binids->start.inl; fms += binids->start.crl;
    fms += binids->stop.inl; fms += binids->stop.crl;

    iopar.set( SeisPacketInfo::sBinIDs, fms );
    iopar.set( SeisPacketInfo::sNrTrcs, nrwr );
    iopar.set( SeisTrcInfo::sStartTime, starttime * 1000 );
    iopar.set( SeisTrcInfo::sNrSamples, trace_size );
    iopar.set( SeisTrcInfo::sSampIntv, ((double)dt) * 0.001 );
}
