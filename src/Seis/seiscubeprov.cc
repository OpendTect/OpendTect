/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seiscubeprov.h"

#include "arrayndimpl.h"
#include "seistrc.h"
#include "seisselectionimpl.h"
#include "seisread.h"
#include "seisbuf.h"
#include "seisbounds.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "ioobj.h"
#include "ioman.h"
#include "errh.h"


static const IOObj* nullioobj = 0;

SeisMSCProvider::SeisMSCProvider( const MultiID& id )
	: rdr_(*new SeisTrcReader(nullioobj))
{
    IOObj* ioobj = IOM().get( id );
    rdr_.setIOObj( ioobj );
    delete ioobj;
    init();
}


SeisMSCProvider::SeisMSCProvider( const IOObj& ioobj )
	: rdr_(*new SeisTrcReader(&ioobj))
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
    intofloats_ = workstarted_ = false;
    errmsg_ = "";
    estnrtrcs_ = -2;
    reqmask_ = 0;
    bufidx_ = -1;
    trcidx_ = -1;
}


SeisMSCProvider::~SeisMSCProvider()
{
    rdr_.close();
    for ( int idx=0; idx<tbufs_.size(); idx++ )
	tbufs_[idx]->deepErase();
    deepErase( tbufs_ );
    delete &rdr_;
    delete reqmask_;
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
    { 
	reqstepout_.row = is2D() ? 0 : i; 
	reqstepout_.col = c; 
	delete reqmask_;
	reqmask_ = 0;
    }
    else
    { 
	desstepout_.row = is2D() ? 0 : i; 
	desstepout_.col = c; 
    }
}


void SeisMSCProvider::setStepout( Array2D<bool>* mask )
{
    if ( !mask ) return;
    
    setStepout( (mask->info().getSize(0)-1)/2, 
		(mask->info().getSize(1)-1)/2, true );
    reqmask_ = mask;
}


void SeisMSCProvider::setSelData( Seis::SelData* sd )
{
    rdr_.setSelData( sd );
}


/* Strategy:
   1) try going to next in already buffered traces: doAdvance()
   2) if not possible, read new trace.
   3) if !doAdvance() now, we're buffering
   */

SeisMSCProvider::AdvanceState SeisMSCProvider::advance()
{
    if ( !workstarted_ && !startWork() )
    {
	if ( errmsg_.isEmpty() ) errmsg_ = rdr_.errMsg();
	if ( errmsg_.isEmpty() ) errmsg_ = "No valid data found";
	return Error;
    }

    if ( doAdvance() )
	return NewPosition;
    else if ( readstate_ == ReadErr )
	return Error;
    else if ( readstate_ == ReadAtEnd )
	return EndReached;

    SeisTrc* trc = new SeisTrc;
    int res = readTrace( *trc );
    if ( res < 1 )
    {
	delete trc;
	readstate_ = res==0 ? ReadAtEnd : ReadErr;
	return advance();
    }

    trc->data().handleDataSwapping();

    SeisTrcBuf* addbuf = tbufs_.isEmpty() ? 0 : tbufs_[ tbufs_.size()-1 ];
    if ( is2D() && trc->info().new_packet )
	addbuf = 0;
    if ( !is2D() && addbuf && 
	 addbuf->get(0)->info().binid.inl != trc->info().binid.inl )
	addbuf = 0;	

    if ( !addbuf )
    {
	addbuf = new SeisTrcBuf( false );
	tbufs_ += addbuf;
    }

    addbuf->add( trc );

    return doAdvance() ? NewPosition : Buffering;
}


int SeisMSCProvider::comparePos( const SeisMSCProvider& mscp ) const
{
    if ( &mscp == this )
	return 0;

    if ( is2D() && mscp.is2D() )
    {
	const int mynr = getTrcNr();
	const int mscpsnr = mscp.getTrcNr();
	if ( mynr == mscpsnr )
	    return 0;
	return mynr > mscpsnr ? 1 : -1;
    }

    const BinID mybid = getPos();
    const BinID mscpsbid = mscp.getPos();
    if ( mybid == mscpsbid )
	return 0;

    if ( mybid.inl != mscpsbid.inl )
	return mybid.inl > mscpsbid.inl ? 1 : -1;

    return mybid.crl > mscpsbid.crl ? 1 : -1;
}


int SeisMSCProvider::estimatedNrTraces() const
{
    if ( estnrtrcs_ != -2 ) return estnrtrcs_;
    estnrtrcs_ = -1;
    if ( !rdr_.selData() )
	return is2D() ? estnrtrcs_ : (int) SI().sampling(false).hrg.totalNr();

    estnrtrcs_ = rdr_.selData()->expectedNrTraces( is2D() );
    return estnrtrcs_;
}


bool SeisMSCProvider::startWork()
{
    if ( !prepareWork() ) return false;

    workstarted_ = true;
    rdr_.forceFloatData( intofloats_ );
    PtrMan<Seis::Bounds> bds = rdr_.getBounds();
    const bool is2d = is2D();
    if ( bds )
    {
	stepoutstep_.row = is2d ? 1 : bds->step( true );
	stepoutstep_.col = is2d ? bds->step( true ): bds->step( false );
    }
    if ( reqstepout_.row > desstepout_.row ) desstepout_.row = reqstepout_.row;
    if ( reqstepout_.col > desstepout_.col ) desstepout_.col = reqstepout_.col;

    if ( rdr_.selData() && !rdr_.selData()->isAll() )
    {
	Seis::SelData* newseldata = rdr_.selData()->clone();
	BinID so( desstepout_.row, desstepout_.col );
	bool doextend = so.inl > 0 || so.crl > 0;
	if ( is2d )
	{
	    so.inl = 0;
	    doextend = doextend && newseldata->type() == Seis::Range;
	    if ( newseldata->type() == Seis::Table )
		newseldata->setIsAll( true );
	}

	if ( doextend )
	{
	    BinID bid( stepoutstep_.row, stepoutstep_.col );
	    newseldata->extendH( so, &bid );
	}

	rdr_.setSelData( newseldata );
    }

    SeisTrc* trc = new SeisTrc;
    int rv = readTrace( *trc );
    while ( rv > 1 )
	rv = readTrace( *trc );

    if ( rv < 0 )
	{ errmsg_ = rdr_.errMsg(); return false; }
    else if ( rv == 0 )
	{ errmsg_ = "No valid/selected trace found"; return false; }

    SeisTrcBuf* newbuf = new SeisTrcBuf( false );
    tbufs_ += newbuf;
    newbuf->add( trc );
    
    pivotidx_ = 0; pivotidy_ = 0;
    readstate_ = ReadOK;
    return true;
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
		msg += trc.info().nr;
	    else
		trc.info().binid.fill( msg.buf() + 6 );
	    msg += ": "; msg += rdr_.errMsg();
	    ErrMsg( msg );
	}
    }
}


BinID SeisMSCProvider::getPos() const
{
    return bufidx_==-1 ? BinID(-1,-1) :
			 tbufs_[bufidx_]->get(trcidx_)->info().binid; 
}


int SeisMSCProvider::getTrcNr() const
{
    return !is2D() || bufidx_==-1 ? -1 :
				    tbufs_[bufidx_]->get(trcidx_)->info().nr;
}


SeisTrc* SeisMSCProvider::get( int deltainl, int deltacrl )
{
    if ( bufidx_==-1 )
	return 0;
    if ( abs(deltainl)>desstepout_.row || abs(deltacrl)>desstepout_.col )
	return 0;

    BinID bidtofind( deltainl*stepoutstep_.row, deltacrl*stepoutstep_.col );
    bidtofind += !is2D() ? tbufs_[bufidx_]->get(trcidx_)->info().binid :
		 BinID( bufidx_, tbufs_[bufidx_]->get(trcidx_)->info().nr );
    
    int idx = mMIN( mMAX(0,bufidx_+deltainl), tbufs_.size()-1 ); 
    while ( !is2D() )
    {
	const int inldif = tbufs_[idx]->get(0)->info().binid.inl-bidtofind.inl;
	if ( !inldif )
	    break;
	if ( deltainl*inldif < 0 )
	    return 0;
	idx += deltainl>0 ? -1 : 1;
    }

    const int idy = tbufs_[idx]->find( bidtofind, is2D() );
    return idy<0 ? 0 : tbufs_[idx]->get(idy);
}


SeisTrc* SeisMSCProvider::get( const BinID& bid )
{
    if ( bufidx_==-1 || !stepoutstep_.row || !stepoutstep_.col )
	return 0;

    RowCol biddif( bid ); 
    biddif -= tbufs_[bufidx_]->get(trcidx_)->info().binid;

    RowCol delta( biddif ); delta /= stepoutstep_; 
    RowCol check( delta  ); check *= stepoutstep_;

    if ( biddif != check )
	return 0;
    
    return get( delta.row, delta.col );
}


// Distances to box borders: 0 on border, >0 outside, <0 inside.
#define mCalcBoxDistances(idx,idy,stepout) \
    const BinID curbid = is2D() ? \
	    BinID( idx, tbufs_[idx]->get(idy)->info().nr ) : \
	    tbufs_[idx]->get(idy)->info().binid; \
    const BinID pivotbid = is2D() ? \
	    BinID( pivotidx_, tbufs_[pivotidx_]->get(pivotidy_)->info().nr ) : \
	    tbufs_[pivotidx_]->get(pivotidy_)->info().binid; \
    RowCol bidstepout( stepout ); bidstepout *= stepoutstep_; \
    const int bottomdist mUnusedVar = pivotbid.inl-curbid.inl-bidstepout.row; \
    const int topdist mUnusedVar = curbid.inl-pivotbid.inl-bidstepout.row; \
    const int leftdist mUnusedVar = pivotbid.crl-curbid.crl-bidstepout.col; \
    const int rightdist mUnusedVar = curbid.crl-pivotbid.crl-bidstepout.col;
   

bool SeisMSCProvider::isReqBoxFilled() const
{
    for ( int idy=0; idy<=2*reqstepout_.col; idy++ )
    { 
	for ( int idx=0; idx<=2*reqstepout_.row; idx++ )
	{
	    if ( !reqmask_ || reqmask_->get(idx,idy) )
	    {
		if ( !get(idx-reqstepout_.row, idy-reqstepout_.col) )
		    return false;
	    }
	}
    }
    return true;
}


bool SeisMSCProvider::doAdvance()
{
    while ( true )
    {
	bufidx_=-1; trcidx_=-1;

	// Remove oldest traces no longer needed from buffer.
	while ( !tbufs_.isEmpty() ) 
	{
	    if ( pivotidx_ < tbufs_.size() )
	    {
		mCalcBoxDistances(0,0,desstepout_);
		
		if ( bottomdist<0 || (bottomdist==0 && leftdist<=0) )
		    break;
	    }

	    if ( !tbufs_[0]->isEmpty() )
	    {
		SeisTrc* deltrc = tbufs_[0]->remove(0);
		delete deltrc;
		if ( pivotidx_ == 0 )
		    pivotidy_--;
	    }

	    if ( tbufs_[0]->isEmpty() )
	    {
		delete tbufs_.removeSingle(0);
		pivotidx_--; 
	    }
	}

	// If no traces left in buffer (e.g. at 0-stepouts), ask next trace.
	if ( tbufs_.isEmpty() )
	    return false;
	
	// If last trace not beyond desired box, ask next trace if available.
	if ( readstate_!=ReadAtEnd && readstate_!=ReadErr )
	{
	    const int lastidx = tbufs_.size()-1;
	    const int lastidy = tbufs_[lastidx]->size()-1;
	    mCalcBoxDistances(lastidx,lastidy,desstepout_);

	    if ( topdist<0 || (topdist==0 && rightdist<0) )
		return false;
	}

	// Store current pivot position for external reference.
	bufidx_=pivotidx_; trcidx_=pivotidy_;
	
	// Determine next pivot position to consider.
	pivotidy_++;
	if ( pivotidy_ == tbufs_[pivotidx_]->size() )
	{
	    pivotidx_++; pivotidy_ = 0;
	}

	// Report stored pivot position if required box valid.
	if ( isReqBoxFilled() )
	    return true;
    }
}



class TrcDataLoader : public Executor
{
public:
TrcDataLoader( SeisTrcReader* rdr, Array2D<SeisTrc*>& arr,
		const HorSampling& hs, bool is2d )
    : Executor("Data Loader")
    , rdr_(rdr),arr_(arr),hs_(hs)
    , nrdone_(0)
    , is2d_(is2d)
{
}

od_int64 totalNr() const
{ return hs_.totalNr(); }

od_int64 nrDone() const
{ return nrdone_; }

const char* message() const
{ return "Reading Steering traces"; }

int nextStep()
{
    if ( !rdr_ )
	return ErrorOccurred();

    SeisTrc* trc = new SeisTrc;

    const int res = rdr_->get( trc->info() );
    if ( res == -1 ) { delete trc; return ErrorOccurred(); }
    if ( res == 0 ) { delete trc; return Finished(); }
    if ( res == 2 ) { delete trc; return MoreToDo(); }
    else if ( rdr_->get(*trc) )
    {
	const BinID bid = trc->info().binid;
	const int inlidx = is2d_ ? 0 : hs_.inlIdx( bid.inl );
	const int crlidx = hs_.crlIdx( is2d_ ? trc->info().nr : bid.crl );
	arr_.set( inlidx, crlidx, trc );
    }
    else
	delete trc;

    nrdone_++;
    return MoreToDo();
}

    SeisTrcReader*		rdr_;
    Array2D<SeisTrc*>&		arr_;
    const HorSampling&		hs_;
    od_int64			nrdone_;
    bool			is2d_;

};


SeisFixedCubeProvider::SeisFixedCubeProvider( const MultiID& key )
    : cs_(false)
    , data_(0)
    , ioobj_(IOM().get(key))
{
}


SeisFixedCubeProvider::~SeisFixedCubeProvider()
{
    clear();
    delete ioobj_;
}

const char* SeisFixedCubeProvider::errMsg() const
{ return errmsg_.buf(); }

void SeisFixedCubeProvider::clear()
{
    if ( !data_ )
	return;

    for ( int idx=0; idx<data_->info().getSize(0); idx++ )
	for ( int idy=0; idy<data_->info().getSize(1); idy++ )
	    delete data_->get( idx, idy );

    delete data_;
    data_ = 0;
}

bool SeisFixedCubeProvider::isEmpty() const
{ return !data_; }

bool SeisFixedCubeProvider::readData( const CubeSampling& cs, TaskRunner* tr )
{ return readData( cs, 0, tr ); }

#define mErrRet(s) { errmsg_ = s; return false; }

bool SeisFixedCubeProvider::readData( const CubeSampling& cs,
				    const LineKey* lk, TaskRunner* tr )
{
    if ( !ioobj_ )
	mErrRet( "Failed to find the input dataset" )

    PtrMan<SeisTrcReader> seisrdr = new SeisTrcReader( ioobj_ );
    seisrdr->prepareWork();

    cs_ = cs;
    Seis::RangeSelData* sd = new Seis::RangeSelData( cs_ );
    if ( lk )
	sd->lineKey() = *lk;

    seisrdr->setSelData( sd );

    clear();
    data_ = new Array2DImpl<SeisTrc*>( cs_.hrg.nrInl(), cs_.hrg.nrCrl() );
    for ( int idx=0; idx<data_->info().getSize(0); idx++ )
	for ( int idy=0; idy<data_->info().getSize(1); idy++ )
	    data_->set( idx, idy, 0 );

    PtrMan<TrcDataLoader> loader = new TrcDataLoader( seisrdr, *data_, cs_.hrg,
	    					      lk );
    const bool res = TaskRunner::execute( tr, *loader );
    if ( !res )
	mErrRet( "Failed to read input dataset" )

    return true;
}


const SeisTrc* SeisFixedCubeProvider::getTrace( int trcnr ) const
{ return getTrace( BinID(0,trcnr) ); }

const SeisTrc* SeisFixedCubeProvider::getTrace( const BinID& bid ) const
{
    if ( !data_ || !cs_.hrg.includes(bid) )
	return 0;

    return data_->get( cs_.inlIdx(bid.inl), cs_.crlIdx(bid.crl) );
}
