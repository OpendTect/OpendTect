/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-1-1998
 * FUNCTION : Seismic data reader
-*/

static const char* rcsID = "$Id: seisread.cc,v 1.31 2004-08-25 14:25:57 bert Exp $";

#include "seisread.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "executor.h"
#include "iostrm.h"
#include "streamconn.h"
#include "survinfo.h"
#include "binidselimpl.h"
#include "errh.h"


#define mUndefPtr(clss) ((clss*)0xdeadbeef) // Like on AIX. Nothing special.


SeisTrcReader::SeisTrcReader( const IOObj* ioob )
	: SeisStoreAccess(ioob)
    	, outer(mUndefPtr(BinIDRange))
    	, fetcher(0)
    	, tbuf(0)
{
    init();
}

#define mDelOuter if ( outer != mUndefPtr(BinIDRange) ) delete outer

SeisTrcReader::~SeisTrcReader()
{
    mDelOuter;
    delete tbuf;
    delete fetcher;
}


void SeisTrcReader::init()
{
    foundvalidinl = foundvalidcrl =
    new_packet = inforead = needskip = prepared = forcefloats = false;
    prev_inl = mUndefIntVal;
    if ( tbuf ) tbuf->deepErase();
    mDelOuter; outer = mUndefPtr(BinIDRange);
    delete fetcher; fetcher = 0;
    curlinenr = -1;
}


bool SeisTrcReader::prepareWork()
{
    if ( !ioobj )
    {
	errmsg = "Info for input seismic data not found in Object Manager";
	return false;
    }
    if ( (is2d && !lgrp) || (!is2d && !trl) )
    {
	errmsg = "No data interpreter available for '";
	errmsg += ioobj->name(); errmsg += "'";
	return false;
    }

    Conn* conn = 0;
    if ( !is2d )
    {
	conn = openFirst();
	if ( !conn )
	{
	    errmsg = "Cannot open data files for '";
	    errmsg += ioobj->name(); errmsg += "'";
	    return false;
	}
    }

    if ( !initRead(conn) )
    {
	delete conn;
	return false;
    }

    return (prepared = true);
}



void SeisTrcReader::startWork()
{
    outer = 0;
    if ( is2d )
    {
	tbuf = new SeisTrcBuf( true );
	return;
    }

    if ( forcefloats )
    {
	for ( int idx=0; idx<trl->componentInfo().size(); idx++ )
	    trl->componentInfo()[idx]->datachar = DataCharacteristics();
    }

    trl->setSelData( seldata );
    if ( trl->inlCrlSorted() && seldata )
    {
	outer = new BinIDRange;
	if ( !seldata->fill(*outer) )
	    { delete outer; outer = 0; }
    }
}


bool SeisTrcReader::isMultiConn() const
{
    return !is2d && ioobj && ioobj->hasConnType(StreamConn::sType)
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
	if ( iostrm && isMultiConn() )
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
    if ( is2d ) return true;

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


int SeisTrcReader::get( SeisTrcInfo& ti )
{
    if ( !prepared && !prepareWork() )
	return -1;
    else if ( outer == mUndefPtr(BinIDRange) )
	startWork();

    if ( is2d )
	return get2D(ti);

    bool needsk = needskip; needskip = false;
    if ( needsk && !trl->skip() )
	return nextConn( ti );

    if ( !trl->readInfo(ti) )
	return nextConn( ti );

    ti.stack_count = 1;
    ti.new_packet = false;

    if ( mIsUndefInt(prev_inl) )
	prev_inl = ti.binid.inl;
    else if ( prev_inl != ti.binid.inl )
    {
	foundvalidcrl = false;
	prev_inl = ti.binid.inl;
	ti.new_packet = true;
    }

    const int selres = seldata ? seldata->selRes(ti.binid) : 0;
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
    if ( !isEmpty(seldata) )
    {
	int selres = seldata->selRes( nrtrcs );
	if ( selres > 1 )
	    return 0;
	if ( selres == 1 )
	    return trl->skip() ? 2 : nextConn( ti );
    }

    // Hey, the trace is (believe it or not) actually selected!
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
    if ( !prepared && !prepareWork() )
	return false;
    else if ( outer == mUndefPtr(BinIDRange) )
	startWork();
    if ( is2d )
	return get2D(trc);

    if ( !trl->read(trc) )
    {
	errmsg = trl->errMsg();
	trl->skip();
	return false;
    }
    return true;
}


bool SeisTrcReader::mkNextFetcher()
{
    bool issingline = seldata && seldata->linekey_ != "";
    if ( curlinenr != -1 && issingline )
	return false;

    curlinenr++; tbuf->deepErase();
    if ( issingline )
    {
	bool found = false;
	const int nrlines = lgrp->nrLines();
	for ( ; curlinenr<nrlines; curlinenr++ )
	{
	    if ( lgrp->lineKey(curlinenr) == seldata->linekey_ )
		{ found = true; break; }
	}
	if ( !found )
	{
	    errmsg = "Selected line not found in line set";
	    return false;
	}
    }
    fetcher = lgrp->lineFetcher( curlinenr, *tbuf, seldata );
    return fetcher;
}


bool SeisTrcReader::readNext2D()
{
    if ( tbuf->size() ) return true;

    int res = fetcher->doStep();
    if ( res == Executor::ErrorOccurred )
    {
	errmsg = fetcher->message();
	return false;
    }
    else if ( res == 0 )
	{ delete fetcher; fetcher = 0; }

    return tbuf->size();
}


#define mNeedNextFetcher() (tbuf->size() == 0 && !fetcher)


bool SeisTrcReader::get2D( SeisTrcInfo& ti )
{
    if ( mNeedNextFetcher() && !mkNextFetcher() )
	return false;
    if ( !readNext2D() )
	return false;

    if ( inforead && tbuf->size() )
	delete tbuf->remove(0);

    inforead = true;
    SeisTrcInfo& trcti = tbuf->get( 0 )->info();
    trcti.new_packet = prev_inl != trcti.nr;
    ti = trcti;
    prev_inl = ti.nr;
    return true;
}


bool SeisTrcReader::get2D( SeisTrc& trc )
{
    if ( mNeedNextFetcher() && !get2D(trc.info()) )
	return false;
    else if ( !inforead && !readNext2D() )
	return false;

    inforead = false;
    const SeisTrc* buftrc = tbuf->get( 0 );
    if ( !buftrc )
	{ pErrMsg("Huh"); return false; }
    trc.info() = buftrc->info();
    trc.copyDataFrom( *buftrc, -1, forcefloats );
    delete tbuf->remove(0);
    return true;
}


int SeisTrcReader::nextConn( SeisTrcInfo& ti )
{
    new_packet = false;
    if ( !isMultiConn() ) return 0;

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

    if ( seldata && seldata->isPositioned()
	         && iostrm->directNumberMultiConn() )
    {
	if ( !binidInConn(seldata->selRes(ti.binid)) )
	    return nextConn( ti );
    }

    if ( rv == 2 )	new_packet = true;
    else		ti.new_packet = true;

    return rv;
}


void SeisTrcReader::trySkipConns()
{
    if ( !isMultiConn() || !seldata || !seldata->isPositioned() )
	return;
    mDynamicCastGet(IOStream*,iostrm,ioobj)
    if ( !iostrm || !iostrm->directNumberMultiConn() ) return;

    BinID binid;

    if ( seldata->type_ == SeisSelData::Range )
	binid.crl = seldata->crlrg_.start;
    else if ( seldata->table_.totalSize() == 0 )
	return;
    else
	binid.crl = seldata->table_.firstPos().crl;

    do
    {
	binid.inl = iostrm->connNr();
	if ( binidInConn(seldata->selRes(binid)) )
	    return;

    } while ( iostrm->toNextConnNr() );
}


bool SeisTrcReader::binidInConn( int r ) const
{
    return r == 0 || !trl->inlCrlSorted() || r%256 != 2;
}


void SeisTrcReader::fillPar( IOPar& iopar ) const
{
    SeisStoreAccess::fillPar( iopar );
    if ( seldata )	seldata->fillPar( iopar );
    else		SeisSelData::removeFromPar( iopar );
}
