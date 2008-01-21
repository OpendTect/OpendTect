/*+
* COPYRIGHT: (C) dGB Beheer B.V.
* AUTHOR   : A.H. Bril
* DATE     : 28-1-1998
* FUNCTION : Seismic data writer
-*/

#include "seiswrite.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seisselection.h"
#include "seis2dline.h"
#include "seispsioprov.h"
#include "seispswrite.h"
#include "seispscubetr.h"
#include "executor.h"
#include "iostrm.h"
#include "iox.h"
#include "ioman.h"
#include "iodir.h"
#include "separstr.h"
#include "iopar.h"


#define mCurLineKey (lkp ? lkp->lineKey() : seldata->lineKey())


SeisTrcWriter::SeisTrcWriter( const IOObj* ioob, const LineKeyProvider* l )
	: SeisStoreAccess(ioob)
    	, lineauxiopar(*new IOPar)
	, lkp(l)
{
    init();
}


SeisTrcWriter::SeisTrcWriter( const char* fnm, bool isps )
	: SeisStoreAccess(fnm,isps)
    	, lineauxiopar(*new IOPar)
	, lkp(0)
{
    init();
}


void SeisTrcWriter::init()
{
    putter = 0; pswriter = 0;
    nrtrcs = nrwritten = 0;
    prepared = false;
}


SeisTrcWriter::~SeisTrcWriter()
{
    close();
    delete &lineauxiopar;
}


bool SeisTrcWriter::close()
{
    bool ret = true;
    if ( putter )
	{ ret = putter->close(); if ( !ret ) errmsg = putter->errMsg(); }

    delete putter; putter = 0;
    delete pswriter; pswriter = 0;
    psioprov = 0;
    ret &= SeisStoreAccess::close();

    return ret;
}


bool SeisTrcWriter::prepareWork( const SeisTrc& trc )
{
    if ( !ioobj )
    {
	errmsg = "Info for output seismic data not found in Object Manager";
	return false;
    }

    if ( !psioprov && ((is2d && !lset) || (!is2d && !trl)) )
    {
	errmsg = "No data storer available for '";
	errmsg += ioobj->name(); errmsg += "'";
	return false;
    }
    if ( is2d && !lkp && ( !seldata || seldata->lineKey().isEmpty() ) )
    {
	errmsg = "Internal: 2D seismic can only be stored if line key known";
	return false;
    }

    if ( is2d && !psioprov )
    {
	if ( !next2DLine() )
	    return false;
    }
    else if ( psioprov )
    {
	const char* psstorkey = ioobj->fullUserExpr(Conn::Write);
	pswriter = is2d ? psioprov->make2DWriter( psstorkey, mCurLineKey )
	    		: psioprov->make3DWriter( psstorkey );
	if ( !pswriter )
	{
	    errmsg = "Cannot open Pre-Stack data store for write";
	    return false;
	}
	pswriter->usePar( ioobj->pars() );
	if ( !is2d )
	{
	    if ( !ioobj->pars().find(SeisPSIOProvider::sKeyCubeID) )
	    {
		IOM().to( ioobj->key() );
		BufferString nm( "{" ); nm += ioobj->name(); nm += "}";
		IOX* iox = new IOX( nm );
		iox->setTranslator( mTranslKey(SeisPSCubeSeisTrc) );
		iox->setGroup( mTranslGroupName(SeisTrc) );
		iox->acquireNewKey();
		ioobj->pars().set( SeisPSIOProvider::sKeyCubeID, iox->key() );
		IOM().dirPtr()->commitChanges( ioobj );
		iox->setOwnKey( ioobj->key() );
		IOM().dirPtr()->addObj( iox, true );
	    }
	}
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
    if ( !conn || conn->bad() )
    {
	errmsg = "Cannot write to ";
	errmsg += ioobj->fullUserExpr(false);
	delete conn;
	return false;
    }

    strl()->cleanUp();
    if ( !strl()->initWrite(conn,trc) )
    {
	errmsg = strl()->errMsg();
	delete conn;
	return false;
    }

    return true;
}


bool SeisTrcWriter::ensureRightConn( const SeisTrc& trc, bool first )
{
    bool neednewconn = !curConn3D();

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


bool SeisTrcWriter::next2DLine()
{
    LineKey lk = mCurLineKey;
    if ( !attrib.isEmpty() )
	lk.setAttrName( attrib );
    BufferString lnm = lk.lineName();
    if ( lnm.isEmpty() )
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
    if ( seldata && seldata->selRes( trc.info().binid ) )
	return true;

    if ( is2d )
    {
	if ( !put2D(trc) )
	    return false;
    }
    else if ( psioprov )
    {
	if ( !pswriter )
	    return false;
	if ( !pswriter->put(trc) )
	{
	    errmsg = pswriter->errMsg();
	    return false;
	}
    }
    else
    {
	if ( !ensureRightConn(trc,false) )
	    return false;
	else if ( !strl()->write(trc) )
	{
	    errmsg = strl()->errMsg();
	    strl()->close(); delete trl; trl = 0;
	    return false;
	}
    }

    nrwritten++;
    return true;
}


bool SeisTrcWriter::isMultiConn() const
{
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    return iostrm && iostrm->isMulti();
}
