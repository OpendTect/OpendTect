/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2007
-*/

static const char* rcsID = "$Id: seiscubeprov.cc,v 1.4 2007-04-06 15:04:59 cvsjaap Exp $";

#include "seismscprov.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "seisread.h"
#include "seisbuf.h"
#include "survinfo.h"
#include "ioobj.h"
#include "ioman.h"
#include "errh.h"


SeisMSCProvider::SeisMSCProvider( const MultiID& id )
	: rdr_(*new SeisTrcReader(0))
{
    IOObj* ioobj = IOM().get( id );
    rdr_.setIOObj( ioobj );
    delete ioobj;
    init();
}


SeisMSCProvider::SeisMSCProvider( const IOObj* ioobj )
	: rdr_(*new SeisTrcReader(ioobj))
{
    init();
}


SeisMSCProvider::SeisMSCProvider( const char* fnm )
	: rdr_(*new SeisTrcReader(fnm))
{
    init();
}


void SeisMSCProvider::init()
{
    readstate_ = NeedStart;
    stepoutstep_.inl = SI().inlStep(); stepoutstep_.crl = SI().crlStep();
    seldata_ = 0;
    intofloats_ = workstarted_ = false;
    errmsg_ = 0;
    estnrtrcs_ = -2;
    curbuf_ = 0;
}


SeisMSCProvider::~SeisMSCProvider()
{
    rdr_.close();
    deepErase( tbufs_ );
    delete seldata_;
    delete &rdr_;
}


bool SeisMSCProvider::prepareWork()
{
    const bool prepared = rdr_.isPrepared() ? true : rdr_.prepareWork();
    if ( !prepared )
	errmsg_ = rdr_.errMsg();

    return prepared;
}


bool SeisMSCProvider::is2D() const
{
    return rdr_.is2D();
}


void SeisMSCProvider::setStepout( int i, int c, bool req )
{
    if ( req )
	reqstepout_.r() = i; reqstepout_.c() = c;
    else
	desstepout_.r() = i; desstepout_.c() = c;
}


bool SeisMSCProvider::startWork()
{
    if ( !prepareWork() ) return false;

    workstarted_ = true;
    rdr_.forceFloatData( intofloats_ );
    if ( reqstepout_.r() > desstepout_.r() ) desstepout_.r() = reqstepout_.r();
    if ( reqstepout_.c() > desstepout_.c() ) desstepout_.c() = reqstepout_.c();

    delete seldata_; seldata_ = 0;
    if ( !rdr_.selData() || rdr_.selData()->all_ )
	return true;

    seldata_ = new SeisSelData( *rdr_.selData() );
    SeisSelData* newseldata = new SeisSelData( *rdr_.selData() );
    BinID so( desstepout_.r(), desstepout_.c() );
    bool doextend = so.inl > 0 || so.crl > 0;
    if ( is2D() )
    {
	so.inl = 0;
	doextend = doextend && newseldata->type_ == Seis::Range;
	if ( so.crl && newseldata->type_ == Seis::Table )
	    newseldata->all_ = true;
    }

    if ( doextend )
	newseldata->extend( so, &stepoutstep_ );

    rdr_.setSelData( newseldata );
    SeisTrc* trc = new SeisTrc;
    int rv = readTrace( *trc );
    while ( rv > 1 )
	rv = readTrace( *trc );

    if ( rv < 0 )
	{ errmsg_ = rdr_.errMsg(); return false; }
    else if ( rv == 0 )
	{ errmsg_ = "No valid/selected trace found"; return false; }

    SeisTrcBuf* newbuf = new SeisTrcBuf;
    tbufs_ += newbuf;
    newbuf->add( trc );
    
    pivotidx_ = 0;

    return true;
}


int SeisMSCProvider::estimatedNrTraces() const
{
    if ( estnrtrcs_ != -2 ) return estnrtrcs_;
    estnrtrcs_ = -1;
    if ( !rdr_.selData() )
	return is2D() ? estnrtrcs_ : SI().sampling().hrg.totalNr();

    estnrtrcs_ = rdr_.selData()->expectedNrTraces( is2D() );
    return estnrtrcs_;
}


int SeisMSCProvider::comparePos( const SeisMSCProvider& req ) const
{
    if ( &req == this )
	return 0;

    if ( is2D() && req.is2D() )
    {
	if ( curnr_ == req.curnr_ )
	    return 0;
	return curnr_ > req.curnr_ ? 1 : -1;
    }

    if ( curbid_ == req.curbid_ )
	return 0;

    if ( curbid_.inl != req.curbid_.inl )
	return curbid_.inl > req.curbid_.inl ? 1 : -1;

    return curbid_.crl > req.curbid_.crl ? 1 : -1;
}


int SeisMSCProvider::readTrace( SeisTrc& trc )
{
    while ( true )
    {
	const int rv = rdr_.get( trc.info() );

	switch ( rv )
	{
	case 1:		break;
	case -1:	errmsg_ = rdr_.errMsg();	return -1;
	case 0:						return 0;
	case 2:
	default:					return 2;
	}

	if ( rdr_.get(trc) )
	    return 1;
	else
	{
	    BufferString msg( "Trace " );
	    if ( is2D() )
		msg += lastread_->info().nr;
	    else
		lastread_->info().binid.fill( msg.buf() + 6 );
	    msg += ": "; msg += rdr_.errMsg();
	    ErrMsg( msg );
	}
    }
}


#define mRet(act,rv) { act; return rv; }

/* Strategy:
   1) try going to next in already buffered traces: doAdvance()
   2) if not possible, read new trace.
   3) if !doAdvance() now, we're buffering
   */

SeisMSCProvider::AdvanceState SeisMSCProvider::advance()
{
    if ( !workstarted_ && !startWork() )
	{ errmsg_ = rdr_.errMsg(); return AdvErr; }

    if ( doAdvance() )
	return NewPosition;
    else if ( readstate_ == ReadErr )
	return AdvErr;
    else if ( readstate_ == AtEnd )
	return EndReached;

    SeisTrc* trc = new SeisTrc;
    int res = readTrace( *trc );
    if ( res < 1 )
	delete trc;
    if ( res < 0 )
    {
	readstate_ = res == 0 ? AtEnd : ReadErr;
	return advance();
    }

    trc->data().handleDataSwapping();

    SeisTrcBuf* addbuf = tbufs_[ tbufs_.size()-1 ];
    bool neednewbuf = is2D() && trc->info().new_packet;
    if ( !is2D() )
	neednewbuf = addbuf->size() > 0
	          && addbuf->get(0)->info().binid.inl != trc->info().binid.inl;
    if ( neednewbuf )
    {
	if ( is2D() )
	{
	    tbufs_[0]->deepErase();
	    deepErase( tbufs_ );
	}
	addbuf = new SeisTrcBuf;
	tbufs_ += addbuf;
    }

    addbuf->add( trc );
    return doAdvance() ? NewPosition : Buffering;
}


// Distances to box borders: 0 on border, >0 outside, <0 inside.
#define mCalcBoxDistances(curidx,pivotidx,stepout) \
    const BinID& curbid = tbufs_[curidx].info().binid; \
    const BinID& pivotbid = tbufs_[pivotidx].info().binid; \
    const int bottomdist = pivotbid.inl - curbid.inl - stepout.r(); \
    const int topdist = curbid.inl - pivotbid.inl - stepout.r(); \
    const int leftdist = pivotbid.crl - curbid.crl - stepout.c(); \
    const int rightdist = curbid.crl - pivotbid.crl - stepout.c(); 


bool SeisMSCProvider::gapInReqBox( int pivotidx, bool upwards ) const
{
    int emptybins = (2*reqstepout_.r()+1)*reqstepout_.c() + reqstepout_.r();
    
    const int dir = upwards ? 1 : -1;
    const int bufsz = tbufs_.size();

    for ( int idx=pivotidx+dir; idx>=0 && idx<bufsz; idx+=dir )
    {
	mCalcBoxDistances(idx,pivotidx,reqstepout_);

	if ( bottomdist<=0 && topdist<=0 && leftdist<=0 && rightdist<=0 )
	    emptybins--;

	if ( bottomdist>0 || bottomdist==0 && leftdist>=0 )
	    break;
	if ( topdist>0 || topdist==0 && rightdist>=0 ) 
	    break;
    }

    return emptybins;
}


bool SeisMSCProvider::doAdvance()
{
    newposidx_= -1;  

    while ( true )
    {
	// Remove leading traces no longer needed from buffer.
	while ( tbufs_.size() ) 
	{
	    mCalcBoxDistances(0,pivotidx_,desstepout_);

	    if ( bottomdist<0 || bottomdist==0 && leftdist<0 )
		break;

	    delete tbufs_.remove(0);
	    pivotidx_--; 
	}

	// If no traces left in buffer (e.g. at 0-stepouts), ask next trace.
	if ( !tbufs_.size() )
	    return false;
	
	// If last trace not beyond desired box, ask next trace if available.
	if ( readstate_!=AtEnd && readstate_!=ReadError )
	{
	    mCalcBoxDistances(tbufs_.size()-1,pivotidx_,desstepout_);

	    if ( topdist<0 || topdist==0 && rightdist<0 )
		return false;
	}

	// Skip position if required box will remain incomplete.
	if ( gapInReqBox(pivotidx_,false) || gapInReqBox(pivotidx_,true) )
	{
	    pivotidx_++;
	    continue;
	}

	// Report readiness of new position.
	newposidx_ = pivotidx_;
	pivotidx_++;
	return true;
    }
}
