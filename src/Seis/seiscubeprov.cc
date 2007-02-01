/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2007
-*/

static const char* rcsID = "$Id: seiscubeprov.cc,v 1.1 2007-02-01 18:13:01 cvsbert Exp $";

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
    state_ = DataIncomplete;
    stepoutstep_.inl = SI().inlStep(); stepoutstep_.crl = SI().crlStep();
    curpos_.c() = 0; curpos_.r() = -1;
    seldata_ = 0;
    intofloats_ = workstarted_ = false;
    errmsg_ = 0;
    estnrtrcs_ = -2;
    tbufs_ += new SeisTrcBuf;
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
	reqstepout_.c() = i; reqstepout_.r() = c;
    else
	desstepout_.c() = i; desstepout_.r() = c;
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
	int res = curnr_ > req.curnr_ ? 1 : -1;
	if ( crlrev_ ) res = -res;
	return res;
    }

    if ( curbid_ == req.curbid_ )
	return 0;

    if ( curbid_.inl != req.curbid_.inl )
    {
	int res = curbid_.inl > req.curbid_.inl ? 1 : -1;
	if ( inlrev_ ) res = -res;
	return res;
    }

    int res = curbid_.crl > req.curbid_.crl ? 1 : -1;
    if ( crlrev_ ) res = -res;

    return res;
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


#define mRet(st,act) { atc; state_ = st; return state_; }

SeisMSCProvider::State SeisMSCProvider::advance()
{
    if ( state_ == NoMoreData || state_ == Error )
	return state_;
    else if ( !workstarted_ && !startWork() )
	mRet(Error,errmsg_ = rdr_.errMsg())

    if ( atend_ )
	return advanceAtEnd();

    SeisTrc* newtrc = new SeisTrc;
    int res = readTrace( *newtrc );
    if ( res < 1 )
	delete newtrc;
    if ( res < 0 )
	mRet(Error,)
    else if ( res == 0 )
	{ state_ = AtEnd; return advanceAtEnd(); }

    return addTrc( newtrc );
}


SeisMSCProvider::State SeisMSCProvider::addTrc( SeisTrc* trc )
{
    trc->data().handleDataSwapping();

    SeisTrcBuf* curbuf = tbufs_[ tbufs_.size()-1 ];
    bool neednewbuf = is2D() && trc->info().new_packet;
    if ( !is2D() )
	neednewbuf = curbuf->size() > 0
	          && curbuf->get(0)->info().binid.inl != trc->info().binid.inl;
    if ( neednewbuf )
	curbuf = newBuf( trc );
    else
	curbuf->add( trc );

    return handleFreshTrace();
}


SeisTrcBuf* SeisMSCProvider::newBuf( SeisTrc* trc )
{
    if ( is2D() )
    {
	tbufs_[0]->deepErase();
	deepErase( tbufs_ );
    }

    SeisTrcBuf* newbuf = new SeisTrcBuf;
    tbufs_ += newbuf;
    newbuf->add( trc );

    if ( is2D() )
	return newbuf;

    const int lowestinl = trc->info().binid.inl
			- (2 * desstepout_.c() * stepoutstep_.inl);
    while ( tbufs_[0].get(0)->info().binid.inl < lowestinl )
    {
	SeisTrcBuf* oldbuf = new SeisTrcBuf;
	oldbuf->deepErase();
	tbufs_ -= oldbuf;
	delete oldbuf;
    }
}

SeisMSCProvider::State SeisMSCProvider::handleFreshTrace()
{
    const int startrowoffs = reqstepout_.crl;
    curpos_.r() = curbuf->size() - reqstepout_.r() - 1;
    if ( curpos_.r() < 0 )
	return DataIncomplete;

    // Check whether all required traces present
    for ( int irow=
}
