/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jan 2007
-*/


#include "seismscprov.h"

#include "arrayndimpl.h"
#include "horsubsel.h"
#include "posinfo2d.h"
#include "seisbuf.h"
#include "seisprovider.h"
#include "seisseldata.h"
#include "seistrc.h"
#include "uistrings.h"


Seis::MSCProvider::MSCProvider( const DBKey& id )
    : provmine_(true)
{
    prov_ = Provider::create( id, &uirv_ );
    if ( !prov_ )
    {
	prov_ = &Provider3D::dummy();
	mNonConst(provmine_) = false;
    }
}


Seis::MSCProvider::MSCProvider( Provider& prov )
    : prov_(&prov)
    , provmine_(false)
{
}


Seis::MSCProvider::~MSCProvider()
{
    for ( int idx=0; idx<tbufs_.size(); idx++ )
	tbufs_[idx]->deepErase();
    deepErase( tbufs_ );
    delete reqmask_;
    if ( provmine_ )
	delete prov_;
}


bool Seis::MSCProvider::is2D() const
{
    return prov_->is2D();
}


BufferString Seis::MSCProvider::name() const
{
    return prov_->name();
}


void Seis::MSCProvider::setStepout( int i, int c, bool req )
{
    auto& so = req ? reqstepout_ : desstepout_;
    so.lineNr() = is2D() ? 0 : i;
    so.trcNr() = c;
    if ( req )
	deleteAndZeroPtr( reqmask_ );
    if ( prov_ )
	prov_->is2D() ? prov_->as2D()->setStepout( c )
		      : prov_->as3D()->setStepout( so );
}


void Seis::MSCProvider::setStepout( Array2D<bool>* mask )
{
    deleteAndZeroPtr( reqmask_ );
    if ( mask )
    {
	setStepout( (mask->getSize(0)-1)/2,
		    (mask->getSize(1)-1)/2, true );
	reqmask_ = mask;
    }
}


void Seis::MSCProvider::setSelData( SelData* sd )
{
    prov_->setSelData( sd );
}


void Seis::MSCProvider::setSelData( const SelData& sd )
{
    prov_->setSelData( sd );
}


void Seis::MSCProvider::setZExtension( const z_rg_type& zrg )
{
    prov_->setZExtension( zrg );
}


/* Strategy:
   1) try going to next in already buffered traces: doAdvance()
   2) if not possible, read new trace.
   3) if !doAdvance() now, we're buffering
   */

Seis::MSCProvider::AdvanceState Seis::MSCProvider::advance()
{
    if ( !prov_ || (!workstarted_ && !startWork()) )
	return Error;

    if ( doAdvance() )
	return NewPosition;
    else if ( !uirv_.isOK() )
	return Error;
    else if ( atend_ )
	return EndReached;

    SeisTrc* trc = new SeisTrc;
    if ( !readTrace(*trc) )
	{ delete trc; return atend_ ? EndReached : Error; }

    trc->data().handleDataSwapping();

    SeisTrcBuf* addbuf = tbufs_.isEmpty() ? 0 : tbufs_[ tbufs_.size()-1 ];
    if ( addbuf && addbuf->get(0)->info().lineNr() != trc->info().lineNr() )
	addbuf = 0;

    if ( !addbuf )
    {
	addbuf = new SeisTrcBuf( false );
	tbufs_ += addbuf;
    }

    addbuf->add( trc );

    return doAdvance() ? NewPosition : Buffering;
}


int Seis::MSCProvider::comparePos( const MSCProvider& oth ) const
{
    if ( &oth == this )
	return 0;

    const auto& tbuf = *tbufs_[bufidx_];
    int startval = tbuf.get(0)->info().trcNr();
    int stopval = tbuf.get(tbuf.size()-1)->info().trcNr();

    int bufidx = oth.bufidx_;

    int startothval = oth.tbufs_[bufidx]->get(0)->info().trcNr();
    int stopothval = oth.tbufs_[bufidx]->
			    get(oth.tbufs_[bufidx]->size()-1)->info().trcNr();

    bool arebothreversed = (startval > stopval) &&
					(startothval > stopothval);

    if ( is2D() && oth.is2D() )
    {
	const auto mynr = curTrcNr();
	const auto othsnr = oth.curTrcNr();
	if ( mynr == othsnr )
	    return 0;
	return ( (mynr > othsnr) && !arebothreversed ) ? 1 : -1;
    }

    const BinID mybid = curBinID();
    const BinID othbid = oth.curBinID();
    if ( mybid == othbid )
	return 0;

    if ( mybid.inl() != othbid.inl() )
	return mybid.inl() > othbid.inl() ? 1 : -1;

    return ( mybid.crl() > othbid.crl() ) ? 1 : -1;
}


od_int64 Seis::MSCProvider::estimatedNrTraces() const
{
    return prov_ ? prov_->totalNr( true ) : -1;
}


bool Seis::MSCProvider::startWork()
{
    if ( !prov_ )
	return false;
    if ( prov_->isEmpty() )
	{ uirv_.set( tr("No input data available") ); return false; }

    if ( reqstepout_.lineNr() > desstepout_.lineNr() )
	desstepout_.lineNr() = reqstepout_.lineNr();
    if ( reqstepout_.trcNr() > desstepout_.trcNr() )
	desstepout_.trcNr() = reqstepout_.trcNr();

    prov_->forceFPData( intofloats_ );
    if ( prov_->is2D() )
    {
	auto& prov2d = *prov_->as2D();
	prov2d.setStepout( desstepout_.trcNr() );
	stepoutstep_.crl() = prov2d.trcNrStep( 0 );
    }
    else
    {
	auto& prov3d = *prov_->as3D();
	prov3d.setStepout( desstepout_ );
	stepoutstep_ = prov3d.binIDStep();
    }

    SeisTrc* trc = new SeisTrc;
    if ( !readTrace(*trc) )
	return false;

    SeisTrcBuf* newbuf = new SeisTrcBuf( false );
    tbufs_ += newbuf;
    newbuf->add( trc );

    pivotidx_ = 0; pivotidy_ = 0;
    workstarted_ = true;
    return true;
}



bool Seis::MSCProvider::readTrace( SeisTrc& trc )
{
    if ( !prov_ )
	return false;

    uirv_ = prov_->getNext( trc );
    if ( uirv_.isOK() )
    {
	if ( prov_->is2D() && trc.info().geomID() != curgeomid_ )
	{
	    const auto& prov2d = *prov_->as2D();
	    curgeomid_ = trc.info().geomID();
	    stepoutstep_.trcNr() = prov2d.trcNrStep(
					prov2d.lineIdx(curgeomid_) );
	}
	return true;
    }

    atend_ = isFinished( uirv_ );
    if ( atend_ )
	uirv_.setEmpty();
    return false;
}


Pos::IdxPair Seis::MSCProvider::curPos() const
{
    if ( !is2D() )
	return curBinID();
    else
    {
	const auto curb2d = curBin2D();
	return IdxPair( curb2d.lineNr(), curb2d.trcNr() );
    }
}


SeisTrc* Seis::MSCProvider::curTrc()
{
    return bufidx_ < 0 ? nullptr : tbufs_[bufidx_]->get( trcidx_ );
}


BinID Seis::MSCProvider::curBinID() const
{
    const auto* trc = curTrc();
    return trc ? trc->info().binID() : BinID::udf();
}


Bin2D Seis::MSCProvider::curBin2D() const
{
    const auto* trc = curTrc();
    return trc ? trc->info().bin2D() : Bin2D();
}


Pos::GeomID Seis::MSCProvider::curGeomID() const
{
    if ( !is2D() )
	return Pos::GeomID::get3D();

    if ( !prov_ )
	return Pos::GeomID();

    return prov_->as2D()->curGeomID();
}


int Seis::MSCProvider::curTrcNr() const
{
    const auto* trc = curTrc();
    return trc ? trc->info().trcNr() : Bin2D().trcNr();
}


SeisTrc* Seis::MSCProvider::getAt( int deltatrcnr )
{
    if ( !is2D() )
	{ pErrMsg("Use 3D version"); return getAt( 0, deltatrcnr ); }
    if ( bufidx_ < 0 )
	return nullptr;
    if ( abs(deltatrcnr)>desstepout_.trcNr() )
	return nullptr;

    auto& tbuf = *tbufs_[bufidx_];
    const auto trcnrtofind = curTrcNr() + deltatrcnr*stepoutstep_.trcNr();
    const auto trcidx = tbuf.find( Bin2D(curgeomid_,trcnrtofind) );
    return trcidx<0 ? 0 : tbuf.get( trcidx );
}


SeisTrc* Seis::MSCProvider::getAt( int deltainl, int deltacrl )
{
    if ( is2D() )
	{ pErrMsg("Use 2D version"); return getAt( deltacrl ); }
    if ( bufidx_ < 0 )
	return nullptr;
    if ( abs(deltainl)>desstepout_.inl() || abs(deltacrl)>desstepout_.crl() )
	return nullptr;

    BinID bidtofind( deltainl*stepoutstep_.inl(), deltacrl*stepoutstep_.crl() );
    bidtofind += curBinID();

    int ibuf = mMIN( mMAX(0,bufidx_+deltainl), tbufs_.size()-1 );
    while ( true )
    {
	const int inldif =
	    tbufs_[ibuf]->get(0)->info().inl()-bidtofind.inl();
	if ( !inldif )
	    break;
	if ( deltainl*inldif < 0 )
	    return nullptr;
	ibuf += deltainl>0 ? -1 : 1;
    }

    auto& tbuf = *tbufs_[ibuf];
    const int trcidx = tbuf.find( bidtofind );
    return trcidx<0 ? nullptr : tbuf.get( trcidx );
}


SeisTrc* Seis::MSCProvider::getFor( const Bin2D& b2d )
{
    if ( bufidx_==-1 || !stepoutstep_.trcNr() || b2d.geomID() != curgeomid_ )
	return nullptr;

    const auto tnrdif = b2d.trcNr() - curTrcNr();
    auto delta = tnrdif; delta /= stepoutstep_.trcNr();
    auto check = delta;  check *= stepoutstep_.trcNr();
    if ( tnrdif != check )
	return 0;

    return getAt( delta );
}


SeisTrc* Seis::MSCProvider::getFor( const BinID& bid )
{
    if ( bufidx_==-1 || !stepoutstep_.inl() || !stepoutstep_.crl() )
	return nullptr;

    const BinID biddif( bid - curBinID() );

    BinID delta( biddif ); delta = delta / BinID(stepoutstep_);
    BinID check( delta  ); check = check * BinID(stepoutstep_);
    if ( biddif != check )
	return 0;

    return getAt( delta.inl(), delta.crl() );
}


// Distances to box borders: 0 on border, >0 outside, <0 inside.
#define mCalcBoxDistances(idx,idy,stepout) \
    const IdxPair curip = tbufs_[idx]->get(idy)->info().idxPair(); \
    const IdxPair pivotip =tbufs_[pivotidx_]->get(pivotidy_)->info().idxPair();\
    IdxPair ipstepout( stepout ); \
    ipstepout.lineNr() = ipstepout.lineNr() * stepoutstep_.lineNr(); \
    ipstepout.trcNr() = ipstepout.trcNr() * stepoutstep_.trcNr(); \
    const auto bottomdist mUnusedVar = \
	pivotip.inl()-curip.lineNr()-ipstepout.lineNr(); \
    const auto topdist mUnusedVar = \
	curip.inl()-pivotip.lineNr()-ipstepout.lineNr(); \
    const auto leftdist mUnusedVar = \
	pivotip.crl()-curip.trcNr()-ipstepout.trcNr(); \
    const auto rightdist mUnusedVar = \
	curip.crl()-pivotip.trcNr()-ipstepout.trcNr();


bool Seis::MSCProvider::isReqBoxFilled() const
{
    if ( is2D() )
    {
	for ( int itnr=0; itnr<=2*reqstepout_.trcNr(); itnr++ )
	{
	    if ( !reqmask_ || reqmask_->get(0,itnr) )
	    {
		if ( !getAt(itnr-reqstepout_.trcNr()) )
		    return false;
	    }
	}
    }
    else
    {
	for ( int icrl=0; icrl<=2*reqstepout_.crl(); icrl++ )
	{
	    for ( int iinl=0; iinl<=2*reqstepout_.inl(); iinl++ )
	    {
		if ( !reqmask_ || reqmask_->get(iinl,icrl) )
		{
		    if ( !getAt(iinl-reqstepout_.inl(),icrl-reqstepout_.crl()) )
			return false;
		}
	    }
	}
    }
    return true;
}


bool Seis::MSCProvider::doAdvance()
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
	if ( !atend_ )
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


bool Seis::MSCProvider::toNextPos()
{
    uirv_.setEmpty();
    bool atnext = false;
    while ( !atnext )
    {
	switch ( advance() )
	{
	case Seis::MSCProvider::EndReached:
	    return false;
	case Seis::MSCProvider::Error:
	{
	    if ( uirv_.isOK() )
		uirv_.add(uiStrings::phrErrDuringRead(uiStrings::sSeismics()));
	    return false;
	}
	case Seis::MSCProvider::NewPosition:
	    { atnext = true; break; }
	default:
	    break;
	}
    }

    return true;
}


bool Seis::advance( ObjectSet<Seis::MSCProvider>& provs, uiRetVal& uirv )
{
    uirv.setEmpty();
    if ( provs.isEmpty() )
	{ uirv.set( mINTERNAL("No seismics") ); return false; }

    const bool is2d = provs.first()->is2D();
    BinID curbid; Bin2D curb2d;
    for ( int iprov=0; iprov<provs.size(); iprov++ )
    {
	Seis::MSCProvider& prov = *provs[iprov];
	if ( iprov == 0 )
	{
	    if ( !prov.toNextPos() )
		{ uirv.set( prov.errMsg() ); return false; }
	    if ( is2d )
		curb2d = prov.curBin2D();
	    else
		curbid = prov.curBinID();
	}

	if ( is2d )
	{
	    Bin2D provb2d = prov.curBin2D();
	    if ( mIsUdf(provb2d.trcNr()) )
	    {
		if ( !prov.toNextPos() )
		    { uirv.set( prov.errMsg() ); return false; }
		provb2d = prov.curBin2D();
	    }

	    while ( provb2d.geomID() != curb2d.geomID()
		 || provb2d.trcNr() < curb2d.trcNr() )
	    {
		if ( !prov.toNextPos() )
		    { uirv.set( prov.errMsg() ); return false; }
		provb2d = prov.curBin2D();
	    }
	    if ( provb2d.geomID() != curb2d.geomID()
		|| provb2d.trcNr() > curb2d.trcNr() )
		{ iprov = -1; continue; }
	}
	else
	{
	    BinID provbid = prov.curBinID();
	    if ( mIsUdf(provbid.inl()) )
	    {
		if ( !prov.toNextPos() )
		    { uirv.set( prov.errMsg() ); return false; }
		provbid = prov.curBinID();
	    }

	    while ( provbid < curbid )
	    {
		if ( !prov.toNextPos() )
		    { uirv.set( prov.errMsg() ); return false; }
		provbid = prov.curBinID();
	    }
	    if ( provbid > curbid )
		{ iprov = -1; continue; }
	}
    }

    return true;
}
