/*+
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* AUTHOR   : A.H. Bril
* DATE     : 28-1-1998
-*/
static const char* rcsID = "$Id: seiswrite.cc,v 1.56 2010-09-29 08:25:21 cvssatyaki Exp $";

#include "seiswrite.h"
#include "keystrs.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seisselection.h"
#include "seis2dline.h"
#include "seis2dlineio.h"
#include "seispsioprov.h"
#include "seispswrite.h"
#include "seispscubetr.h"
#include "executor.h"
#include "iostrm.h"
#include "iox.h"
#include "ioman.h"
#include "separstr.h"
#include "surv2dgeom.h"
#include "iopar.h"


#define mCurLineKey (lkp ? lkp->lineKey() : seldata->lineKey())
const char* SeisTrcWriter::sKeyWriteBluntly() { return "Write bluntly"; }


SeisTrcWriter::SeisTrcWriter( const IOObj* ioob, const LineKeyProvider* l )
	: SeisStoreAccess(ioob)
    	, lineauxiopar(*new IOPar)
	, lkp(l)
	, worktrc(*new SeisTrc)
	, makewrready(true)
{
    init();
}


SeisTrcWriter::SeisTrcWriter( const char* fnm, bool is2d, bool isps )
	: SeisStoreAccess(fnm,is2d,isps)
    	, lineauxiopar(*new IOPar)
	, lkp(0)
	, worktrc(*new SeisTrc)
	, makewrready(true)
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
    delete &worktrc;
}


bool SeisTrcWriter::close()
{
    bool ret = true;
    if ( putter )
	{ ret = putter->close(); if ( !ret ) errmsg_ = putter->errMsg(); }

    if ( is2D() )
    {
	LineKey linekey = lkp ? lkp->lineKey() : seldata ? seldata->lineKey()
	    						 : "";
	const int lineidx = lset ? lset->indexOf(linekey) : -1;
	if ( lineidx>=0 )
	{
	    const bool hasgeom = 
		PosInfo::POS2DAdmin().hasLineSet( lset->name() ) &&
		PosInfo::POS2DAdmin().hasLine(linekey.lineName(),lset->name());
	    if ( !hasgeom )
	    {
		PosInfo::POS2DAdmin().setCurLineSet( lset->name() );
		PosInfo::Line2DData l2dd( linekey.lineName() );
		if ( lset->getGeometry( lineidx, l2dd ) )
		    PosInfo::POS2DAdmin().setGeometry( l2dd );
	    }
	}
    }

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
	errmsg_ = "Info for output seismic data not found in Object Manager";
	return false;
    }

    if ( !psioprov && ((is2d && !lset) || (!is2d && !trl)) )
    {
	errmsg_ = "No data storer available for '";
	errmsg_ += ioobj->name(); errmsg_ += "'";
	return false;
    }
    if ( is2d && !lkp && ( !seldata || seldata->lineKey().isEmpty() ) )
    {
	errmsg_ = "Internal: 2D seismic can only be stored if line key known";
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
	    errmsg_ = "Cannot open Pre-Stack data store for write";
	    return false;
	}
	pswriter->usePar( ioobj->pars() );
	if ( !is2d )
	    SPSIOPF().mk3DPostStackProxy( *ioobj );
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
	{ errmsg_ = "No data from object manager"; return 0; }

    if ( isMultiConn() )
    {
	mDynamicCastGet(IOStream*,iostrm,ioobj)
	iostrm->setConnNr( inl );
    }

    return ioobj->getConn( Conn::Write );
}


bool SeisTrcWriter::start3DWrite( Conn* conn, const SeisTrc& trc )
{
    if ( !conn || conn->bad() || !trl )
    {
	errmsg_ = "Cannot write to ";
	errmsg_ += ioobj->fullUserExpr(false);
	delete conn;
	return false;
    }

    strl()->cleanUp();
    if ( !strl()->initWrite(conn,trc) )
    {
	errmsg_ = strl()->errMsg();
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
		   || (iostrm->isMulti() &&
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
	errmsg_ = "Cannot write to empty line name";
	return false;
    }

    prevlk = lk;
    delete putter;

    IOPar* lineiopar = new IOPar;
    lk.fillPar( *lineiopar, true );

    if ( !datatype.isEmpty() )
	lineiopar->set( sKey::DataType, datatype.buf() );

    lineiopar->merge( lineauxiopar );
    putter = lset->linePutter( lineiopar );
    if ( !putter )
    {
	errmsg_ = "Cannot create 2D line writer";
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
	errmsg_ = putter->errMsg();
    return res;
}



bool SeisTrcWriter::put( const SeisTrc& intrc )
{
    const SeisTrc* trc = &intrc;
    if ( makewrready && !intrc.isWriteReady() )
    {
	intrc.getWriteReady( worktrc );
	trc = &worktrc;
    }
    if ( !prepared ) prepareWork(*trc);

    nrtrcs++;
    if ( seldata && seldata->selRes( trc->info().binid ) )
	return true;

    if ( psioprov )
    {
	if ( !pswriter )
	    return false;
	if ( !pswriter->put(*trc) )
	{
	    errmsg_ = pswriter->errMsg();
	    return false;
	}
    }
    else if ( is2d )
    {
	if ( !put2D(*trc) )
	    return false;
    }
    else
    {
	if ( !ensureRightConn(*trc,false) )
	    return false;
	else if ( !strl()->write(*trc) )
	{
	    errmsg_ = strl()->errMsg();
	    strl()->close(); delete trl; trl = 0;
	    return false;
	}
    }

    nrwritten++;
    return true;
}


void SeisTrcWriter::usePar( const IOPar& iop )
{
    SeisStoreAccess::usePar( iop );
    bool wrbl = !makewrready;
    iop.getYN( sKeyWriteBluntly(), wrbl );
    makewrready = !wrbl;
}


void SeisTrcWriter::fillPar( IOPar& iop ) const
{
    SeisStoreAccess::fillPar( iop );
    iop.setYN( sKeyWriteBluntly(), !makewrready );
}


bool SeisTrcWriter::isMultiConn() const
{
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    return iostrm && iostrm->isMulti();
}
