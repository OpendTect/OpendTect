/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-1-1998
 * FUNCTION : Seismic data reader
-*/

static const char* rcsID = "$Id: seiswrite.cc,v 1.1.1.1 1999-09-03 10:11:27 dgb Exp $";

#include "seiswrite.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "executor.h"
#include "ioobj.h"
#include "separstr.h"
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
    nrsamps = 0;
    dt = 4000;
    ntrcs = 0;
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
	if ( !wr.openFirst() )
	{
	    msg = "Cannot open data storage for write";
	    return -1;
	}
	msg = "Writing global header data";
	return YES;
    }

    if ( !wr.initWrite() )
    {
	msg = wr.errMsg();
	return -1;
    }

    return NO;
}

    SeisTrcWriter&		wr;
    const char*		msg;

};


Executor* SeisTrcWriter::starter()
{
    return new SeisWriteStarter( *this );
}


bool SeisTrcWriter::openFirst()
{
    conn = ioobj->conn( Conn::Write );
    if ( !conn || conn->bad() )
    {
	delete ioobj; ioobj = 0;
	delete conn; conn = 0;
    }
    return conn ? YES : NO;
}


bool SeisTrcWriter::initWrite()
{
    if ( !trl->initWrite(spi,*conn) )
    {
	delete ioobj; ioobj = 0;
	delete conn; conn = 0;
	errmsg = trl->errMsg();
	return NO;
    }

    return YES;
}


bool SeisTrcWriter::put( const SeisTrc& trc )
{
    if ( trc.info().new_packet && !nextConn() )
	return NO;

    if ( !trl->write(trc) )
    {
	errmsg = trl->errMsg();
	return NO;
    }

    if ( ++ntrcs > 1 )
	binids->include( trc.info().binid );
    else
    {
	starttime = trc.info().starttime;
	nrsamps = trc.size();
	dt = trc.info().dt;
	binids->start = trc.info().binid;
	binids->stop = trc.info().binid;
    }

    return YES;
}


bool SeisTrcWriter::nextConn()
{
    if ( !ioobj->multiConn() ) return YES;

    delete conn;
    conn = ioobj->nextConn( Conn::Write );
    if ( !conn || conn->bad() )
    {
	errmsg = ioobj->multiConn() ? "Cannot create new file"
				    : "Last file number used";
	return NO;
    }

    if ( !trl->initWrite(spi,*conn) )
    {
	errmsg = trl->errMsg();
	return NO;
    }

    return YES;
}


void SeisTrcWriter::fillAuxPar( IOPar& iopar ) const
{
    if ( ntrcs < 1 ) return;
    FileMultiString fms;
    fms += binids->start.inl; fms += binids->start.crl;
    fms += binids->stop.inl; fms += binids->stop.crl;

    iopar.set( SeisPacketInfo::sBinIDs, fms );
    iopar.set( SeisPacketInfo::sNrTrcs, ntrcs );
    iopar.set( SeisTrcInfo::sStartTime, starttime * 1000 );
    iopar.set( SeisTrcInfo::sNrSamples, nrsamps );
    iopar.set( SeisTrcInfo::sSampIntv, ((double)dt) / 1000 );
}
