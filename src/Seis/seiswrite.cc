/*+
* COPYRIGHT: (C) dGB Beheer B.V.
* AUTHOR   : A.H. Bril
* DATE     : 28-1-1998
* FUNCTION : Seismic data writer
-*/

#include "seiswrite.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "executor.h"
#include "iostrm.h"
#include "separstr.h"
#include "binidselimpl.h"
#include "iopar.h"

SeisTrcWriter::SeisTrcWriter( const IOObj* ioob )
	: SeisStoreAccess(ioob)
	, binids(*new BinIDRange)
    	, nrtrcs(0)
    	, nrwritten(0)
    	, prepared(false)
{
    binids.start.inl = mUndefIntVal;
}


SeisTrcWriter::~SeisTrcWriter()
{
    delete &binids;
}


bool SeisTrcWriter::prepareWork( const SeisTrc& trc )
{
    if ( !ioobj )
    {
	errmsg = "Info for output seismic data not found in Object Manager";
	return false;
    }
    if ( !trl )
    {
	errmsg = "No data interpreter available for '";
	errmsg += ioobj->name(); errmsg += "'";
	return false;
    }

    mDynamicCastGet(const IOStream*,strm,ioobj)
    if ( !strm || !strm->isMulti() )
	fullImplRemove( *ioobj );

    if ( !ensureRightConn(trc,true) )
    	return false;

    prepared = true;
    return true;
}


Conn* SeisTrcWriter::crConn( int inl, bool first )
{
    if ( !ioobj )
	{ errmsg = "No data from object manager"; return 0; }

    if ( isMultiConn() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj)
	if ( iostrm->directNumberMultiConn() )
	    iostrm->setConnNr( inl );
	else if ( !first )
	    iostrm->toNextConnNr();
    }

    return ioobj->getConn( Conn::Write );
}


bool SeisTrcWriter::startWrite( Conn* conn, const SeisTrc& trc )
{
    trl->cleanUp();
    if ( !conn || conn->bad() )
    {
	errmsg = "Cannot write to ";
	errmsg += ioobj->fullUserExpr(false);
	delete conn;
	return false;
    }

    if ( !trl->initWrite(conn,trc) )
    {
	errmsg = trl->errMsg();
	delete conn;
	return false;
    }

    return true;
}


bool SeisTrcWriter::ensureRightConn( const SeisTrc& trc, bool first )
{
    bool neednewconn = !trl->curConn();

    if ( !neednewconn && isMultiConn() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj)
	neednewconn = trc.info().new_packet
		   || (iostrm->directNumberMultiConn() &&
			iostrm->connNr() != trc.info().binid.inl);
    }

    if ( neednewconn )
    {
	Conn* conn = crConn( trc.info().binid.inl, first );
	if ( !conn || !startWrite(conn,trc) )
	    return false;
    }

    return true;
}


bool SeisTrcWriter::put( const SeisTrc& trc )
{
    if ( !prepared ) prepareWork(trc);

    nrtrcs++;
    if ( seldata )
    {
	if ( seldata->type_ == SeisSelData::TrcNrs )
	{
	    int selres = seldata->selRes(nrtrcs);
	    if ( selres == 1 )
		return true;
	    else if ( selres > 1 )
	    {
		errmsg = "Selected number of traces reached";
		return false;
	    }
	}
	else if ( seldata->selRes( trc.info().binid ) )
	    return true;
    }

    if ( !ensureRightConn(trc,false) )
	return false;
    else if ( !trl->write(trc) )
    {
	errmsg = trl->errMsg();
	trl->close(); delete trl; trl = 0;
	return false;
    }

    nrwritten++;
    if ( mIsUndefInt(binids.start.inl) )
	binids.start = binids.stop = trc.info().binid;
    else
	binids.include( trc.info().binid );

    return true;
}


bool SeisTrcWriter::isMultiConn() const
{
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    return iostrm && iostrm->isMulti();
}


bool SeisTrcWriter::isMultiComp() const
{
    if ( !trl || !trl->componentInfo().size() )
	return false;

    int nrsel = 0;
    for ( int idx=0; idx<trl->componentInfo().size(); idx++ )
	if ( trl->componentInfo()[idx]->destidx >= 0 )
	    nrsel++;

    return nrsel > 1;
}


void SeisTrcWriter::fillAuxPar( IOPar& iopar ) const
{
    if ( !trl || nrwritten < 1 )
	return;

    FileMultiString fms;
    fms += binids.start.inl; fms += binids.start.crl;
    fms += binids.stop.inl; fms += binids.stop.crl;
    iopar.set( SeisPacketInfo::sBinIDs, fms );

    iopar.set( SeisStoreAccess::sNrTrcs, nrwritten );
    iopar.set( SeisTrcInfo::sSamplingInfo, trl->outSD().start,
	    				   trl->outSD().step );
    iopar.set( SeisTrcInfo::sNrSamples, trl->outNrSamples() );
}
