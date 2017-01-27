/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2007
-*/


#include "seismscprov.h"

#include "arrayndimpl.h"
#include "trckeyzsampling.h"
#include "ioobj.h"
#include "dbman.h"
#include "posinfo2d.h"
#include "seisbounds.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisprovider.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "uistrings.h"


Seis::MSCProvider::MSCProvider( const DBKey& id )
{
    prov_ = Provider::create( id, &uirv_ );

    intofloats_ = workstarted_ = atend_ = false;
    estnrtrcs_ = -2;
    reqmask_ = 0;
    bufidx_ = -1;
    trcidx_ = -1;
    curlinenr_ = -1;
}


Seis::MSCProvider::~MSCProvider()
{
    for ( int idx=0; idx<tbufs_.size(); idx++ )
	tbufs_[idx]->deepErase();
    deepErase( tbufs_ );
    delete prov_;
    delete reqmask_;
}


bool Seis::MSCProvider::is2D() const
{
    return prov_ && prov_->is2D();
}


BufferString Seis::MSCProvider::name() const
{
    return prov_ ? prov_->name() : BufferString();
}


void Seis::MSCProvider::setStepout( int i, int c, bool req )
{
    if ( req )
    {
	reqstepout_.row() = is2D() ? 0 : i;
	reqstepout_.col() = c;
	delete reqmask_;
	reqmask_ = 0;
    }
    else
    {
	desstepout_.row() = is2D() ? 0 : i;
	desstepout_.col() = c;
    }
}


void Seis::MSCProvider::setStepout( Array2D<bool>* mask )
{
    if ( !mask ) return;

    setStepout( (mask->info().getSize(0)-1)/2,
		(mask->info().getSize(1)-1)/2, true );
    reqmask_ = mask;
}


void Seis::MSCProvider::setSelData( Seis::SelData* sd )
{
    if ( prov_ )
	prov_->setSelData( sd );
    else
	delete sd;
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
    {
	delete trc;
	return atend_ ? EndReached : Error;
    }

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


int Seis::MSCProvider::comparePos( const MSCProvider& mscp ) const
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

    if ( mybid.inl() != mscpsbid.inl() )
	return mybid.inl() > mscpsbid.inl() ? 1 : -1;

    return mybid.crl() > mscpsbid.crl() ? 1 : -1;
}


int Seis::MSCProvider::estimatedNrTraces() const
{
    if ( estnrtrcs_ == -2 )
    {
	estnrtrcs_ = -1;
	if ( prov_ )
	    estnrtrcs_ = mCast(int,prov_->totalNr());
    }
    return estnrtrcs_;
}


bool Seis::MSCProvider::startWork()
{
    if ( !prov_ )
	return false;

    prov_->forceFPData( intofloats_ );
    if ( prov_->is2D() )
	stepoutstep_ = BinID( 1, 1 );
    else
    {
	mDynamicCastGet( Provider3D*, prov3d, prov_ );
	TrcKeyZSampling cs;
	if ( prov3d->getRanges(cs) )
	    stepoutstep_ = cs.hsamp_.step_;
    }

    if ( reqstepout_.row() > desstepout_.row() )
	desstepout_.row() = reqstepout_.row();
    if ( reqstepout_.col() > desstepout_.col() )
	desstepout_.col() = reqstepout_.col();

    const SelData* sd = prov_->selData();
    if ( sd && !sd->isAll() )
    {
	Seis::SelData* newseldata = sd->clone();
	BinID so( desstepout_.row(), desstepout_.col() );
	bool doextend = so.inl() > 0 || so.crl() > 0;
	if ( is2D() )
	{
	    so.inl() = 0;
	    doextend = doextend && newseldata->type() == Seis::Range;
	    if ( newseldata->type() == Seis::Table )
		newseldata->setIsAll( true );
	}

	if ( doextend )
	{
	    BinID bid( stepoutstep_.row(), stepoutstep_.col() );
	    newseldata->extendH( so, &bid );
	}

	prov_->setSelData( newseldata );
    }

    if ( prov_->is2D() )
	stepoutstep_.crl() = 1;
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
	if ( prov_->is2D() )
	{
	    if ( trc.info().lineNr() != curlinenr_ )
	    {
		stepoutstep_.crl() = 1;
		curlinenr_ = trc.info().lineNr();
		mDynamicCastGet( Provider2D*, prov2d, prov_ );
		PosInfo::Line2DData l2dd;
		prov2d->getGeometryInfo( prov2d->curLineIdx(), l2dd );
		if ( l2dd.size() > 1 )
		{
		    const float avgstep = mCast(float,l2dd.trcNrRange().width()
					  / (l2dd.size()-1));
		    if ( avgstep > 1.5 )
			stepoutstep_.crl() = mNINT32( avgstep );
		}
	    }
	}
	return true;
    }

    atend_ = isFinished( uirv_ );
    if ( atend_ )
	uirv_.setEmpty();
    return false;
}


BinID Seis::MSCProvider::getPos() const
{
    return bufidx_ < 0 ? BinID(-1,-1)
	 : tbufs_[bufidx_]->get(trcidx_)->info().binID();
}


int Seis::MSCProvider::getTrcNr() const
{
    return !is2D() || bufidx_==-1
	? -1 : tbufs_[bufidx_]->get(trcidx_)->info().trcNr();
}


SeisTrc* Seis::MSCProvider::get( int deltainl, int deltacrl )
{
    if ( bufidx_==-1 )
	return 0;
    if ( abs(deltainl)>desstepout_.row() || abs(deltacrl)>desstepout_.col() )
	return 0;

    BinID bidtofind( deltainl*stepoutstep_.row(), deltacrl*stepoutstep_.col() );
    bidtofind += tbufs_[bufidx_]->get(trcidx_)->info().binID();

    int idx = mMIN( mMAX(0,bufidx_+deltainl), tbufs_.size()-1 );
    while ( !is2D() )
    {
	const int inldif =
	    tbufs_[idx]->get(0)->info().lineNr()-bidtofind.inl();
	if ( !inldif )
	    break;
	if ( deltainl*inldif < 0 )
	    return 0;
	idx += deltainl>0 ? -1 : 1;
    }

    const int idy = tbufs_[idx]->find( bidtofind, is2D() );
    return idy<0 ? 0 : tbufs_[idx]->get(idy);
}


SeisTrc* Seis::MSCProvider::get( const BinID& bid )
{
    if ( bufidx_==-1 || !stepoutstep_.row() || !stepoutstep_.col() )
	return 0;

    RowCol biddif( bid );
    biddif -= tbufs_[bufidx_]->get(trcidx_)->info().binID();

    RowCol delta( biddif ); delta /= stepoutstep_;
    RowCol check( delta  ); check *= stepoutstep_;

    if ( biddif != check )
	return 0;

    return get( delta.row(), delta.col() );
}


// Distances to box borders: 0 on border, >0 outside, <0 inside.
#define mCalcBoxDistances(idx,idy,stepout) \
    const BinID curbid = tbufs_[idx]->get(idy)->info().binID(); \
    const BinID pivotbid = tbufs_[pivotidx_]->get(pivotidy_)->info().binID(); \
    RowCol bidstepout( stepout ); bidstepout *= stepoutstep_; \
    const int bottomdist mUnusedVar = \
	pivotbid.inl()-curbid.inl()-bidstepout.row(); \
    const int topdist mUnusedVar = \
	curbid.inl()-pivotbid.inl()-bidstepout.row(); \
    const int leftdist mUnusedVar = \
	pivotbid.crl()-curbid.crl()-bidstepout.col(); \
    const int rightdist mUnusedVar = \
	curbid.crl()-pivotbid.crl()-bidstepout.col();


bool Seis::MSCProvider::isReqBoxFilled() const
{
    for ( int idy=0; idy<=2*reqstepout_.col(); idy++ )
    {
	for ( int idx=0; idx<=2*reqstepout_.row(); idx++ )
	{
	    if ( !reqmask_ || reqmask_->get(idx,idy) )
	    {
		if ( !get(idx-reqstepout_.row(), idy-reqstepout_.col()) )
		    return false;
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


//---- seisfixedcubeprov.cc ...?

namespace Seis
{

class TrcDataLoader : public Executor
{ mODTextTranslationClass(TrcDataLoader);
public:
TrcDataLoader( Seis::Provider& prov, Array2D<SeisTrc*>& arr,
	       const TrcKeySampling& hs, bool is2d )
    : Executor("Data Loader")
    , prov_(prov), arr_(arr), hs_(hs)
    , nrdone_(0)
    , is2d_(is2d)
{
}

od_int64 totalNr() const
{ return hs_.totalNr(); }

od_int64 nrDone() const
{ return nrdone_; }

uiString nrDoneText() const
{ return tr("Positions done"); }

uiString message() const
{ return errmsg_.isEmpty() ? tr("Reading Steering traces") : errmsg_; }

int nextStep()
{
    SeisTrc* trc = new SeisTrc;
    const uiRetVal uirv = prov_.getNext( *trc );
    if ( !uirv.isOK() )
    {
	delete trc;
	if ( isFinished(uirv) )
	    return Finished();

	errmsg_ = uirv;
	return ErrorOccurred();
    }

    const BinID bid = trc->info().binID();
    const int inlidx = is2d_ ? 0 : hs_.inlIdx( bid.inl() );
    const int crlidx = hs_.crlIdx( bid.crl() );
    arr_.set( inlidx, crlidx, trc );
    nrdone_++;
    return MoreToDo();
}

    Seis::Provider&		prov_;
    Array2D<SeisTrc*>&		arr_;
    const TrcKeySampling&	hs_;
    uiString			errmsg_;
    od_int64			nrdone_;
    bool			is2d_;

};

} // namespace Seis


#include "seisfixedcubeprov.h"


SeisFixedCubeProvider::SeisFixedCubeProvider( const DBKey& key )
    : tkzs_(false)
    , data_(0)
    , ioobj_(DBM().get(key))
    , trcdist_(SI().crlDistance())
{
}


SeisFixedCubeProvider::~SeisFixedCubeProvider()
{
    clear();
    delete ioobj_;
}


uiString SeisFixedCubeProvider::errMsg() const
{ return errmsg_; }


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


bool SeisFixedCubeProvider::calcTrcDist( const Pos::GeomID geomid )
{
    trcdist_ = SI().crlDistance();
    const SeisIOObjInfo si( ioobj_->key() );
    if ( !si.is2D() )
	return true;

    BufferStringSet nms;
    si.getComponentNames( nms, geomid );
    if ( nms.size() > 1 && nms.get(1)=="Line dip" )
    {
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,
			Survey::GM().getGeometry(geomid))
	if ( !geom2d )
	{ errmsg_ = tr("Cannot read 2D geometry"); return false; }

	float max;
	geom2d->data().compDistBetwTrcsStats( max, trcdist_ );
	if ( mIsZero(trcdist_,mDefEps) )
	{
	    errmsg_ = tr("Cannot calculate median trace distance");
	    return false;
	}
    }

    return true;
}


bool SeisFixedCubeProvider::readData(const TrcKeyZSampling& cs,
				     TaskRunner* taskr)
{ return readData( cs, mUdfGeomID, taskr ); }


#define mErrRet(s) { errmsg_ = s; return false; }

bool SeisFixedCubeProvider::readData( const TrcKeyZSampling& cs,
				      const Pos::GeomID geomid,
				      TaskRunner* taskr )
{
    if ( !ioobj_ )
	mErrRet( uiStrings::phrCannotFindDBEntry( uiStrings::sInput() ) );

    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( ioobj_->key(), &uirv );
    if ( !prov )
	mErrRet( uirv );

    tkzs_ = cs;
    const bool is2d = !mIsUdfGeomID( geomid );
    Seis::RangeSelData* sd = new Seis::RangeSelData( tkzs_ );
    if ( is2d )
    {
	sd->setGeomID( geomid );
	if ( !calcTrcDist(geomid) )
	    return false;
    }

    prov->setSelData( sd );

    clear();
    data_ = new Array2DImpl<SeisTrc*>( tkzs_.hsamp_.nrInl(),
				       tkzs_.hsamp_.nrCrl() );
    for ( int idx=0; idx<data_->info().getSize(0); idx++ )
	for ( int idy=0; idy<data_->info().getSize(1); idy++ )
	    data_->set( idx, idy, 0 );

    PtrMan<Seis::TrcDataLoader> loader =
	new Seis::TrcDataLoader( *prov, *data_, tkzs_.hsamp_, is2d );
    const bool res = TaskRunner::execute( taskr, *loader );
    if ( !res )
	mErrRet( uiStrings::phrCannotRead( ioobj_->uiName() ) )

    return true;
}


const SeisTrc* SeisFixedCubeProvider::getTrace( int trcnr ) const
{ return getTrace( BinID(0,trcnr) ); }


const SeisTrc* SeisFixedCubeProvider::getTrace( const BinID& bid ) const
{
    if ( !data_ || !tkzs_.hsamp_.includes(bid) )
	return 0;

    return data_->get( tkzs_.inlIdx(bid.inl()), tkzs_.crlIdx(bid.crl()) );
}
