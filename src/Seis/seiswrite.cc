/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-1-1998
 * FUNCTION : Seismic data reader
-*/

static const char* rcsID = "$Id: seiswrite.cc,v 1.6 2001-02-13 17:21:08 bert Exp $";

#include "seiswrite.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "executor.h"
#include "iostrm.h"
#include "separstr.h"
#include "storlayout.h"
#include "binidselimpl.h"
#include "iopar.h"


SeisTrcWriter::SeisTrcWriter( const IOObj* ioob )
	: SeisStorage(ioob)
	, binids(*new BinIDRange)
{
    init();
}


void SeisTrcWriter::init()
{
    binids.start.inl = -999;
    nrwrconn = 0;
    wrinited = false;
}


SeisTrcWriter::~SeisTrcWriter()
{
    delete &binids;
}


bool SeisTrcWriter::isMultiComp() const
{
    if ( !trl || !trl->componentInfo().size() ) return false;
    int nrsel = 0;
    for ( int idx=0; idx<trl->componentInfo().size(); idx++ )
	if ( trl->componentInfo()[idx]->destidx >= 0 ) nrsel++;
    return nrsel > 1;
}


class SeisWriteStarter : public Executor
{
public:

const char* message() const	{ return msg; }
int nrDone() const		{ return 0; }
const char* nrDoneText() const	{ return "Please wait"; }

SeisWriteStarter( SeisTrcWriter& w, const SeisTrc& t )
	: Executor("Seismic Writer Starter")
	, wr(w)
	, msg("Initializing data store")
	, trc(t)
{
}


int nextStep()
{
    if ( !wr.ioObj() )
    {
	msg = "No info for Object Manager";
	return -1;
    }

    if ( !wr.initWrite( trc ) )
    {
	msg = wr.errMsg();
	return -1;
    }

    return 0;
}

    SeisTrcWriter&	wr;
    const SeisTrc&	trc;
    const char*		msg;

};


Executor* SeisTrcWriter::starter( const SeisTrc& trc )
{
    return new SeisWriteStarter( *this, trc );
}


bool SeisTrcWriter::openConn()
{
    if ( !ioobj ) return false;

    delete conn;
    conn = ioobj->getConn( Conn::Write );
    if ( !conn || conn->bad() )
	{ delete conn; conn = 0; }

    return conn ? true : false;
}


bool SeisTrcWriter::initWrite( const SeisTrc& trc )
{
    if ( !trl )
    {
	delete conn; conn = 0;
	if ( errmsg == "" ) errmsg = "Error initialising data store";
	return false;
    }

    if ( !handleConn(trc) ) return false;
    if ( !conn && !openConn() )
    {
	if ( errmsg == "" )
	    errmsg = "No information for output creation";
	return false;
    }
    wrinited = true;
    return trl->initWrite( *conn, trc );
}


bool SeisTrcWriter::handleConn( const SeisTrc& trc )
{
    if ( !ioobj || !ioobj->hasClass(IOStream::classid) ) return true;
    IOStream* iostrm = (IOStream*)ioobj;
    if ( !iostrm->multiConn() ) return conn ? true : openConn();
    const bool connisbidnr = iostrm->directNumberMultiConn();

    const SeisTrcInfo& ti = trc.info();
    bool neednewpacket = !conn || ti.new_packet;
    StorageLayout lyo = trl->storageLayout();
    bool clustcrl = lyo.type() == StorageLayout::Xline;

    if ( !neednewpacket && connisbidnr )
	neednewpacket = iostrm->connNr() != (lyo.type() == StorageLayout::Xline
					    ? ti.binid.crl : ti.binid.inl);

    if ( !neednewpacket ) return true;

    if ( conn && nrwrconn == 0 )
	(void)iostrm->implRemove(); // What if it fails? Can't stop.
    delete conn; conn = 0;
    nrwrconn = 0;
    if ( !iostrm->toNextConnNr() ) return false;

    if ( connisbidnr )
    {
	int nr = clustcrl ? ti.binid.crl : ti.binid.inl;
	while ( nr != iostrm->connNr() && iostrm->toNextConnNr() )
	    ;
	if ( nr != iostrm->connNr() )
	{
	    errmsg = "Output line "; errmsg += nr;
	    errmsg += " is not in valid range for output";
	    return false;
	}
    }

    if ( !openConn() )
    {
	if ( errmsg == "" ) errmsg = "Cannot create file";
	return false;
    }

    return initWrite( trc );
}


bool SeisTrcWriter::put( const SeisTrc& trc )
{
    if ( !trl ) { errmsg = "No translator"; return false; }
    if ( !wrinited )
    {
	Executor* st = starter( trc );
	if ( !st->execute(0) )
	{
	    errmsg = st->message();
	    delete st;
	    return false;
	}
	delete st;
    }
    else if ( !handleConn( trc ) )
	return false;

    if ( binids.start.inl == -999 )
	binids.start = binids.stop = trc.info().binid;
    else
	binids.include( trc.info().binid );

    bool rv = true;
    if ( trcsel && !trcsel->intv.includes(nrtrcs) )
	/* Leave this trace */;
    else if ( trl->write(trc) )
	nrwrconn++;
    else
    {
	errmsg = trl->errMsg();
	delete trl; trl = 0;
	rv = false;
    }

    nrtrcs++;
    return rv;
}


void SeisTrcWriter::fillAuxPar( IOPar& iopar ) const
{
    if ( nrtrcs < 1 ) return;

    FileMultiString fms;
    fms += binids.start.inl; fms += binids.start.crl;
    fms += binids.stop.inl; fms += binids.stop.crl;

    iopar.set( SeisPacketInfo::sBinIDs, fms );
    iopar.set( SeisStorage::sNrTrcs, nrtrcs );
    iopar.set( SeisTrcInfo::sSamplingInfo, trl->componentInfo()[0]->sd.start,
					   trl->componentInfo()[0]->sd.step );
    iopar.set( SeisTrcInfo::sNrSamples, trl->componentInfo()[0]->nrsamples );
}
