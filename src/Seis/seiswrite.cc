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
#include "seis2dline.h"
#include "executor.h"
#include "iostrm.h"
#include "separstr.h"
#include "binidselimpl.h"
#include "iopar.h"

SeisTrcWriter::SeisTrcWriter( const IOObj* ioob, const LineKeyProvider* l )
	: SeisStoreAccess(ioob)
	, binids(*new BinIDRange)
    	, nrtrcs(0)
    	, nrwritten(0)
    	, prepared(false)
    	, lkp(l)
    	, putter(0)
    	, lineauxiopar(*new IOPar)
{
    binids.start.inl = mUndefIntVal;
}


SeisTrcWriter::~SeisTrcWriter()
{
    close();
    delete &binids;
    delete &lineauxiopar;
}


void SeisTrcWriter::close()
{
    delete putter; putter= 0;
    SeisStoreAccess::close();
}


bool SeisTrcWriter::prepareWork( const SeisTrc& trc )
{
    if ( !ioobj )
    {
	errmsg = "Info for output seismic data not found in Object Manager";
	return false;
    }
    if ( (is2d && !lset) || (!is2d && !trl) )
    {
	errmsg = "No data storer available for '";
	errmsg += ioobj->name(); errmsg += "'";
	return false;
    }
    if ( is2d && !lkp && ( !seldata || seldata->linekey_ == "" ) )
    {
	errmsg = "Internal: 2D seismic can only be stored if line key known";
	return false;
    }

    if ( is2d )
    {
	if ( !next2DLine() )
	    return false;
    }
    else
    {
	mDynamicCastGet(const IOStream*,strm,ioobj)
	if ( !strm || !strm->isMulti() )
	    fullImplRemove( *ioobj );

	if ( !ensureRightConn(trc,true) )
	    return false;
    }

    return (prepared = true);
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


bool SeisTrcWriter::start3DWrite( Conn* conn, const SeisTrc& trc )
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
	if ( !conn || !start3DWrite(conn,trc) )
	    return false;
    }

    return true;
}


#define mCurLineKey (lkp ? lkp->lineKey() : seldata->linekey_)


bool SeisTrcWriter::next2DLine()
{
    LineKey lk = lkp ? lkp->lineKey() : seldata->linekey_;
    if ( attrib != "" )
	lk.setAttrName( attrib );
    BufferString lnm = lk.lineName();
    if ( lnm == "" )
    {
	errmsg = "Cannot write to empty line name";
	return false;
    }

    prevlk = lk;
    delete putter;

    IOPar* lineiopar = new IOPar;
    lk.fillPar( *lineiopar, true );
    lineiopar->merge( lineauxiopar );
    putter = lset->linePutter( lineiopar );
    if ( !putter )
    {
	errmsg = "Cannot create 2D line writer";
	return false;
    }

    return true;
}


bool SeisTrcWriter::put2D( const SeisTrc& trc )
{
    if ( !putter ) return false;

    if ( mCurLineKey != prevlk )
    {
	if ( !next2DLine() )
	    return false;
    }

    bool res = putter->put( trc );
    if ( !res )
	errmsg = putter->errMsg();
    return res;
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

    if ( is2d )
    {
	if ( !put2D(trc) )
	    return false;
    }
    else
    {
	if ( !ensureRightConn(trc,false) )
	    return false;
	else if ( !trl->write(trc) )
	{
	    errmsg = trl->errMsg();
	    trl->close(); delete trl; trl = 0;
	    return false;
	}
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
