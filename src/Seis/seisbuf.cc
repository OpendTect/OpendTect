/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisbuf.h"

#include "arrayndimpl.h"
#include "bufstringset.h"
#include "iopar.h"
#include "flatposdata.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "ptrman.h"
#include "seisbufadapters.h"
#include "seispacketinfo.h"
#include "seisread.h"
#include "seisselection.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "sorting.h"
#include "strmprov.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uistrings.h"
#include "unitofmeasure.h"


SeisTrcBuf::SeisTrcBuf( bool ownr )
    : owner_(ownr)
{}


SeisTrcBuf::SeisTrcBuf( const SeisTrcBuf& b )
    : owner_(b.owner_)
{
    b.copyInto( *this );
}


SeisTrcBuf::~SeisTrcBuf()
{
    if ( owner_ )
	deepErase();
}


void SeisTrcBuf::deepErase()
{
    ::deepErase(trcs_);
}


void SeisTrcBuf::insert( SeisTrc* t, int insidx )
{
    for ( int idx=insidx; idx<trcs_.size(); idx++ )
	t = trcs_.replace( idx, t );
    trcs_ += t;
}


void SeisTrcBuf::copyInto( SeisTrcBuf& buf ) const
{
    for ( int idx=0; idx<trcs_.size(); idx++ )
    {
	const SeisTrc* trc = trcs_[idx];
	buf.add( buf.owner_ ? new SeisTrc(*trc) : const_cast<SeisTrc*>(trc) );
    }
}


void SeisTrcBuf::fill( SeisPacketInfo& spi ) const
{
    const int sz = size();
    if ( sz < 1 ) return;
    const SeisTrc* trc = first();
    BinID bid = trc->info().binID();
    const BinID pbid = bid;
    spi.inlrg_.set( mUdf(int), -mUdf(int), 1 );
    spi.crlrg_.set( mUdf(int), -mUdf(int), 1 );
    spi.zrg_.set( mUdf(float), -mUdf(float), trc->info().sampling_.step_ );

    bool doneinl = false, donecrl = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	trc = get( idx ); bid = trc->info().binID();
	spi.inlrg_.include( bid.inl(), false );
	spi.crlrg_.include( bid.crl(), false);
	const SamplingData<float> trcsd = trc->info().sampling_;
	if ( !mIsUdf(trcsd.start_) && !mIsUdf(trcsd.step_) &&
	     !mIsZero(trcsd.step_,mDefEps) )
	{
	    StepInterval<float> zrg(trcsd.start_, trcsd.atIndex(trc->size()-1),
				    trcsd.step_ );
	    spi.zrg_.include( zrg, false );
	}

	if ( !doneinl && bid.inl() != pbid.inl() )
	{ spi.inlrg_.step_ = bid.inl() - pbid.inl(); doneinl = true; }
	if ( !donecrl && bid.crl() != pbid.crl() )
	{ spi.crlrg_.step_ = bid.crl() - pbid.crl(); donecrl = true; }
    }

    if ( spi.inlrg_.step_ < 0 ) spi.inlrg_.step_ = -spi.inlrg_.step_;
    if ( spi.crlrg_.step_ < 0 ) spi.crlrg_.step_ = -spi.crlrg_.step_;
}


void SeisTrcBuf::add( SeisTrcBuf& tb )
{
    for ( int idx=0; idx<tb.size(); idx++ )
    {
	SeisTrc* trc = tb.get( idx );
	add( owner_ && trc ? new SeisTrc(*trc) : trc );
    }
}


Interval<float> SeisTrcBuf::zRange() const
{
    Interval<float> zrg( mUdf(float), mUdf(float) );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( idx > 0 )
	    zrg.include( get(idx)->zRange() );
	else
	    zrg = get(idx)->zRange();
    }
    return zrg;
}


Interval<float> SeisTrcBuf::getZRange4Shifts( const TypeSet<float>& zvals,
					       bool upw ) const
{
    const Interval<float> myzrg = zRange();
    if ( mIsUdf(myzrg.start_) )
	return myzrg;

    Interval<float> zrg( mUdf(float), mUdf(float) );
    for ( const auto& zval : zvals )
    {
	if ( mIsUdf(zval) )
	    continue;

	const float dz = upw ? -zval : zval;

	const Interval<float> curzrg( myzrg.start_ + dz, myzrg.stop_ + dz );
	if ( mIsUdf(zrg.start_) )
	    zrg = curzrg;
	else
	{
	    zrg.include( curzrg.start_, false );
	    zrg.include( curzrg.stop_, false );
	}
    }

    return zrg;
}


void SeisTrcBuf::getShifted( const Interval<float>& zrg,
			     const TypeSet<float>& zvals,
			     bool upward, float udfval, SeisTrcBuf& out ) const
{
    const int nrtrcs = size();
    if ( nrtrcs < 1 || mIsUdf(zrg.start_) || mIsUdf(zrg.stop_) )
	return;

    ZSampling newzrg( zrg );
    newzrg.step_ = first()->info().sampling_.step_;
    const int newnrsamps = newzrg.nrSteps() + 1;
    const float zrest = newnrsamps*newzrg.step_ - zrg.width();
    newzrg.start_ -= zrest * 0.5f;

    for ( int itrc=0; itrc<nrtrcs; itrc++ )
    {
	const SeisTrc& inptrc = *get( itrc );
	const int nrcomps = inptrc.nrComponents();
	auto* newtrc = new SeisTrc( newnrsamps );
	newtrc->setNrComponents( nrcomps );

	newtrc->info() = inptrc.info();
	newtrc->info().sampling_.set( newzrg );
	newtrc->setAll( udfval );

	out.add( newtrc );
	if ( itrc >= zvals.size() )
	    continue;

	const float zshift = zvals.get( itrc );
	const int inptrcsz = inptrc.size();
	for ( int icomp=0; icomp<nrcomps; icomp++ )
	{
	    for ( int isamp=0; isamp<newnrsamps; isamp++ )
	    {
		const float trcz = newtrc->samplePos( isamp );
		const float orgz = upward ? trcz + zshift : trcz - zshift;
		const int inpisamp = inptrc.nearestSample( orgz );
		if ( inpisamp < 0 )
		    newtrc->set( isamp, inptrc.getFirst(icomp), icomp );
		else if ( inpisamp >= inptrcsz )
		    newtrc->set( isamp, inptrc.getLast(icomp), icomp );
		else
		    newtrc->set( isamp, inptrc.getValue(orgz,icomp), icomp );
	    }
	}
    }
}


void SeisTrcBuf::stealTracesFrom( SeisTrcBuf& tb )
{
    for ( int idx=0; idx<tb.size(); idx++ )
    {
	SeisTrc* trc = tb.get( idx );
	add( trc );
    }
    tb.trcs_.erase();
}


bool SeisTrcBuf::isSorted( bool ascending, SeisTrcInfo::Fld fld ) const
{
    const int sz = size();
    if ( sz < 2 ) return true;

    float prevval = (float) first()->info().getValue(fld);
    for ( int idx=1; idx<sz; idx++ )
    {
	float val = (float) get(idx)->info().getValue(fld);
	float diff = val - prevval;
	if ( !mIsZero(diff,mDefEps) )
	{
	    if ( (ascending && diff < 0) || (!ascending && diff > 0) )
		return false;
	}
	prevval = val;
    }

    return true;
}


void SeisTrcBuf::sort( bool ascending, SeisTrcInfo::Fld fld )
{
    const int sz = size();
    if ( sz < 2 || isSorted(ascending,fld) )
	return;

    mAllocVarLenArr( int, idxs, sz );
    mAllocVarLenArr( float, vals, sz );
    const double offs = trcs_[0]->info().getValue( fld );
    for ( int idx=0; idx<sz; idx++ )
    {
	idxs[idx] = idx;
	vals[idx] = (float)(trcs_[idx]->info().getValue( fld ) - offs);
    }
    sort_coupled( mVarLenArr(vals), mVarLenArr(idxs), sz );
    ObjectSet<SeisTrc> tmp;
    for ( int idx=0; idx<sz; idx++ )
	tmp += trcs_[idxs[idx]];

    trcs_.erase();

    for ( int idx=0; idx<sz; idx++ )
	trcs_ += tmp[ascending ? idx : sz - idx - 1];
}


void SeisTrcBuf::sortForWrite( bool is2d )
{
    const int sz = size();
    if ( sz < 2 )
	return;
    else if ( is2d )
	{ sort( true, SeisTrcInfo::TrcNr ); return; }

    sort( true, SeisTrcInfo::BinIDInl );

    SeisTrcBuf singinlbuf( false );
    singinlbuf.add( get(0) );
    ObjectSet<SeisTrc> sortedtrcs;
#define mAddinlTraces() \
    singinlbuf.sort( true, SeisTrcInfo::BinIDCrl ); \
    sortedtrcs.append( singinlbuf.trcs_ ); \
    singinlbuf.trcs_.erase()
    for ( int idx=1; idx<sz; idx++ )
    {
	SeisTrc* trc = get( idx );
	const bool issameinl = trc->info().inl()
				== singinlbuf.get(0)->info().inl();
	if ( issameinl )
	    singinlbuf.add( trc );
	else
	{
	    mAddinlTraces();
	    singinlbuf.add( trc );
	}
    }
    mAddinlTraces();

    trcs_.erase();
    trcs_.append( sortedtrcs );
}


void SeisTrcBuf::enforceNrTrcs( int nrrequired, SeisTrcInfo::Fld fld,
				bool dostack )
{
    SeisTrc* prevtrc = first();
    if ( !prevtrc ) return;

    float prevval = (float) prevtrc->info().getValue( fld );
    int nrwithprevval = 1;
    for ( int idx=1; idx<=size(); idx++ )
    {
	SeisTrc* trc = idx==size() ? 0 : get(idx);
	float val = (float) (trc ? trc->info().getValue( fld ) : 0);

	if ( trc && mIsEqual(prevval,val,mDefEps) )
	{
	    nrwithprevval++;
	    if ( nrwithprevval > nrrequired )
	    {
		if ( dostack )
		{
		    float wt = mCast(float,nrrequired); wt /= nrwithprevval - 1;
		    SeisTrcPropChg( *get(idx-1) ).stack(*trc,false,wt);
		}
		remove(trc); idx--; delete trc;
	    }
	}
	else
	{
	    for ( int inew=nrwithprevval; inew<nrrequired; inew++ )
	    {
		SeisTrc* newtrc = new SeisTrc(*prevtrc);
		newtrc->data().zero();
		add( newtrc );
		idx++;
	    }
	    nrwithprevval = 1;
	}

	prevval = val;
    }
}


float* SeisTrcBuf::getHdrVals( SeisTrcInfo::Fld fld, double& offs )
{
    const int sz = size();
    if ( sz < 1 ) return 0;

    float* ret = new float [sz];
    offs = first()->info().getValue( fld );
    for ( int idx=0; idx<sz; idx++ )
	ret[idx] = (float)(get(idx)->info().getValue( fld ) - offs);

    return ret;
}


void SeisTrcBuf::revert()
{
    int sz = trcs_.size();
    int hsz = sz / 2;
    for ( int idx=0; idx<hsz; idx++ )
	trcs_.swap( sz-idx-1, idx );
}


int SeisTrcBuf::find( const BinID& binid, bool is2d ) const
{
    int sz = size();
    int startidx = probableIdx( binid, is2d );
    int idx = startidx, pos = 0;
    while ( idx<sz && idx>=0 )
    {
	if ( !is2d && ((SeisTrcBuf*)this)->get(idx)->info().binID() == binid )
	    return idx;
	else if ( is2d &&
	      ((SeisTrcBuf*)this)->get(idx)->info().trcNr() == binid.trcNr() )
	    return idx;
	if ( pos < 0 ) pos = -pos;
	else	       pos = -pos-1;
	idx = startidx + pos;
	if ( idx < 0 ) { pos = -pos; idx = startidx + pos; }
	else if ( idx >= sz ) { pos = -pos-1; idx = startidx + pos; }
    }
    return -1;
}


int SeisTrcBuf::find( const SeisTrc* trc, bool is2d ) const
{
    if ( !trc ) return -1;

    int tryidx = probableIdx( trc->info().binID(), is2d );
    if ( trcs_[tryidx] == trc ) return tryidx;

    // Bugger. brute force then
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ((SeisTrcBuf*)this)->get(idx) == trc )
	    return idx;
    }

    return -1;
}


int SeisTrcBuf::probableIdx( const BinID& bid, bool is2d ) const
{
    int sz = size(); if ( sz < 2 ) return 0;
    BinID start = trcs_[0]->info().binID();
    BinID stop = trcs_[sz-1]->info().binID();
    if ( is2d )
    {
	start.row() = stop.row() = 0;
	start.trcNr() = trcs_[0]->info().trcNr();
	stop.trcNr() = trcs_[sz-1]->info().trcNr();
    }

    BinID dist( start.inl() - stop.inl(), start.crl() - stop.crl() );
    if ( !dist.inl() && !dist.crl() )
	return 0;

    int n1  = dist.inl() ? start.inl() : start.crl();
    int n2  = dist.inl() ? stop.inl()  : stop.crl();
    int pos = dist.inl() ? bid.inl()   : bid.crl();

    float fidx = ((sz-1.f) * (pos - n1)) / (n2-n1);
    int idx = mNINT32(fidx);
    if ( idx < 0 ) idx = 0;
    if ( idx >= sz ) idx = sz-1;
    return idx;
}


bool SeisTrcBuf::dump( const char* fnm, bool is2d, bool isps, int icomp ) const
{
    if ( isEmpty() ) return false;

    od_ostream strm( fnm );
    if ( strm.isBad() )
        return false;

    const SeisTrc& trc0 = *first();
    strm << trc0.info().sampling_.start_
	 << ' ' << trc0.info().sampling_.step_ * SI().zDomain().userFactor()
	 << ' ' << trc0.size();

    for ( int itrc=0; itrc<size(); itrc++ )
    {
	strm << od_newline;
	const SeisTrc& trc = *get( itrc );
	if ( !is2d )
	    strm << trc.info().inl() << ' ' << trc.info().crl();
	else
	{
	    BufferString postxt;
	    postxt += trc.info().trcNr(); postxt += " ";
	    postxt += trc.info().coord_.x_; postxt += " ";
	    postxt += trc.info().coord_.y_;
	    strm << postxt;
	}
	if ( isps )
	    strm << ' ' << trc.info().offset_;

	for ( int isamp=0; isamp<trc.size(); isamp++ )
	    strm << ' ' << trc.get( isamp, icomp );
    }

    strm.flush();

    return strm.isOK();
}


struct SeisTrcBufArray2DInfo : public Array2DInfo
{
SeisTrcBufArray2DInfo( const SeisTrcBuf& tb )
    : buf_(tb)
{
}

SeisTrcBufArray2DInfo( const SeisTrcBufArray2DInfo& ai )
    : buf_(ai.buf_)
{
}

ArrayNDInfo* clone() const override
{
    return new SeisTrcBufArray2DInfo(buf_);
}

int getSize( int dim ) const override
{
    if ( dim == 0 )
	return buf_.size();

    const SeisTrc* trc = buf_.first();
    return trc ? trc->size() : 0;
}

// Mandatory functions ... why?
bool setSize( int, int ) override { return false; }
// Are these really necessary?
od_uint64 getMemPos( const int* ) const { return 0; }
bool validPos( const int* pos ) const override
{ return Array2DInfo::validPos(pos); }
od_uint64 getMemPos( int ) const { return 0; }
bool validPos( int ) const { return false; }
od_uint64 getMemPos( int, int ) const { return 0; }
bool validPos( int p0, int p1 ) const override
{ return Array2DInfo::validPos(p0,p1); }

    const SeisTrcBuf&	buf_;

};


// SeisTrcBufArray2D

SeisTrcBufArray2D::SeisTrcBufArray2D( SeisTrcBuf* tbuf, bool mine, int icomp )
    : buf_(tbuf)
    , comp_(icomp)
    , bufmine_(mine)
{
    if ( !buf_ )
    {
	buf_ = new SeisTrcBuf(true);
	bufmine_ = true;
    }

    info_ = new SeisTrcBufArray2DInfo(*buf_);
}


SeisTrcBufArray2D::SeisTrcBufArray2D( const SeisTrcBuf* tbuf, int icomp )
    : buf_(const_cast<SeisTrcBuf*>(tbuf))
    , comp_(icomp)
    , bufmine_(false)
{
    if ( !buf_ )
    {
	buf_ = new SeisTrcBuf(true);
	bufmine_ = true;
    }

    info_ = new SeisTrcBufArray2DInfo(*buf_);
}


SeisTrcBufArray2D::~SeisTrcBufArray2D()
{
    if ( bufmine_ )
	delete buf_;
    delete info_;
}


float SeisTrcBufArray2D::get( int itrc, int isamp ) const
{
    const SeisTrc* trc = buf_->get( itrc );
    return trc && trc->size() > isamp ? trc->get(isamp,comp_) : mUdf(float);
}


void SeisTrcBufArray2D::set( int itrc, int isamp, float val )
{
    SeisTrc* trc = buf_->get( itrc );
    if ( trc && trc->size() > isamp )
	trc->set( isamp, val, comp_ );
}


// SeisTrcBufDataPack

SeisTrcBufDataPack::SeisTrcBufDataPack( SeisTrcBuf* tbuf,
					Seis::GeomType gt,
					SeisTrcInfo::Fld fld, const char* cat,
					const ZDomain::Info& zdom, int icomp )
    : FlatDataPack(cat)
    , gt_(gt)
    , offsettype_(SI().xyInFeet() ? Seis::OffsetType::OffsetFeet
				  : Seis::OffsetType::OffsetMeter)
    , posfld_(fld)
{
    setZDomain( zdom );
    setBuffer( tbuf, gt, fld, icomp, true );
}


SeisTrcBufDataPack::SeisTrcBufDataPack( const SeisTrcBuf& tbuf,
					Seis::GeomType gt,
					SeisTrcInfo::Fld fld, const char* cat,
					const ZDomain::Info& zdom, int icomp )
    : FlatDataPack(cat)
    , gt_(gt)
    , offsettype_(SI().xyInFeet() ? Seis::OffsetType::OffsetFeet
				  : Seis::OffsetType::OffsetMeter)
    , posfld_(fld)
{
    setZDomain( zdom );
    setBuffer( const_cast<SeisTrcBuf*>(&tbuf), gt, fld, icomp, false );
}


SeisTrcBufDataPack::SeisTrcBufDataPack( SeisTrcBuf* tbuf,
					Seis::GeomType gt,
					SeisTrcInfo::Fld fld, const char* cat,
					int icomp )
  : SeisTrcBufDataPack(tbuf,gt,fld,cat,SI().zDomainInfo(),icomp)
{}


SeisTrcBufDataPack::SeisTrcBufDataPack( const SeisTrcBuf& tbuf,
					Seis::GeomType gt,
					SeisTrcInfo::Fld fld, const char* cat,
					int icomp )
  : SeisTrcBufDataPack(tbuf,gt,fld,cat,SI().zDomainInfo(),icomp)
{}


SeisTrcBufDataPack::SeisTrcBufDataPack( const SeisTrcBufDataPack& oth )
    : FlatDataPack(oth.category(),nullptr)
    , offsettype_(oth.offsettype_)
    , azimuthangletype_(oth.azimuthangletype_)
{
    const bool bufisours =
		oth.arr2d_ && ((SeisTrcBufArray2D*)oth.arr2d_)->bufIsMine();
    auto* buf = const_cast<SeisTrcBuf*>( &oth.trcBuf() );
    if ( buf && bufisours )
	buf = buf->clone();

    setZDomain( oth );
    setBuffer( buf, oth.gt_, oth.posfld_, oth.trcBufArr2D().getComp(),
	       bufisours );
    setName( oth.name() );
}


SeisTrcBufDataPack::~SeisTrcBufDataPack()
{
}


SeisTrcBufDataPack&
SeisTrcBufDataPack::setBuffer( SeisTrcBuf* tbuf, Seis::GeomType gt,
			       SeisTrcInfo::Fld fld, int icomp, bool mine )
{
    Threads::Locker lckr( updateLock() );

    posfld_ = fld;
    gt_ = gt;
    const int tbufsz = tbuf ? tbuf->size() : 0;
    FlatPosData& pd = posData();

    delete arr2d_;
    arr2d_ = new SeisTrcBufArray2D( tbuf, mine, icomp );
    if ( !tbuf )
	return *this;

    flds_.setEmpty();
    if ( tbuf->isEmpty() || !tbuf->first()->info().trcKey().isSynthetic() )
    {
	SeisTrcInfo::getAxisCandidates( gt_, flds_ );
	if ( !tbuf->isEmpty() )
	{
	    const float spnr = tbuf->first()->info().refnr_;
	    if ( mIsUdf(spnr) || mIsEqual(spnr,1.f,1e-6f) )
		flds_ -= SeisTrcInfo::RefNr;
	}
    }
    else
	flds_ += fld;

    double ofv;
    float* hdrvals = tbuf->getHdrVals( posfld_, ofv );
    pd.setX1Pos( hdrvals, tbufsz, ofv );
    SeisPacketInfo pinf;
    tbuf->fill( pinf );
    StepInterval<double> zrg;
    assign( zrg, pinf.zrg_ );
    pd.setRange( false, zrg );

    return *this;
}


SeisTrcBufDataPack& SeisTrcBufDataPack::setOffsetType( Seis::OffsetType typ )
{
    offsettype_ = typ;
    return *this;
}


SeisTrcBufDataPack& SeisTrcBufDataPack::setAzimuthAngleType( OD::AngleType typ )
{
    azimuthangletype_ = typ;
    return *this;
}


void SeisTrcBufDataPack::getAltDimKeys( uiStringSet& keys, bool dim0 ) const
{
    if ( !dim0 )
	return;

    for ( const auto& fld : flds_ )
    {
	if ( fld == SeisTrcInfo::TrcNr )
	    keys.add( uiStrings::sTraceNumber() );
	else if ( fld == SeisTrcInfo::RefNr )
	    keys.add( uiStrings::sSPNumber() );
	else if ( fld == SeisTrcInfo::Offset )
	    keys.add( Seis::isOffsetDist(offsetType()) ? uiStrings::sOffset()
						       : uiStrings::sAngle() );
	else if ( fld == SeisTrcInfo::Azimuth )
	    keys.add( uiStrings::sAzimuth() );
	else
	    keys.add( SeisTrcInfo::toUiString(fld) );
    }
}


void SeisTrcBufDataPack::getAltDimKeysUnitLbls( uiStringSet& ss, bool dim0,
				bool abbreviated, bool withparentheses ) const
{
    if ( !dim0 )
	return;

    for ( const auto& fld : flds_ )
    {
	switch( fld )
	{
	    case SeisTrcInfo::BinIDInl: case SeisTrcInfo::BinIDCrl:
	    case SeisTrcInfo::TrcNr:	case SeisTrcInfo::RefNr:
		ss.add( uiString::empty() ); break;
	    case SeisTrcInfo::CoordX:	case SeisTrcInfo::CoordY:
		ss.add( SI().getUiXYUnitString(abbreviated,withparentheses) );
		break;
	    case SeisTrcInfo::Offset:
	    {
		const UnitOfMeasure* uom =
			UnitOfMeasure::offsetUnit( offsetType() );
		ss.add( uom ? uom->getUiLabel(abbreviated,withparentheses)
			    : uiString::empty() );
		break;
	    }
	    case SeisTrcInfo::Azimuth:
	    {
		const UnitOfMeasure* uom =
			UnitOfMeasure::angleUnit( azimuthAngleType() );
		ss.add( uom ? uom->getUiLabel(abbreviated,withparentheses)
			    : uiString::empty() );
		break;
	    }
	    default:
		ss.add( uiString::empty() ); break;
	}
    }
}


double SeisTrcBufDataPack::getAltDimValue( int ikey, bool dim0, int idx ) const
{
    if ( !dim0 )
	return posdata_.position( dim0, idx );

    const SeisTrcBuf& buf = trcBuf();
    return buf.validIdx( idx ) && !flds_.validIdx( idx )
		 ? buf.get(idx)->info().getValue( flds_[ikey] )
		 : FlatDataPack::getAltDimValue( ikey, dim0, idx );
}


bool SeisTrcBufDataPack::dimValuesInInt( const uiString& keystr,
					 bool dim0 ) const
{
    if ( !dim0 || keystr == uiStrings::sSPNumber() )
	return false;

    if ( keystr == uiStrings::sTraceNumber() )
	return true;

    const uiStringSet& flddefstrings = SeisTrcInfo::FldDef().strings();
    const int idx = flddefstrings.indexOf( keystr );
    if ( !flddefstrings.validIdx(idx) )
	return false;

    const SeisTrcInfo::Fld fld = SeisTrcInfo::FldDef().getEnumForIndex(idx);
    return fld == SeisTrcInfo::BinIDInl || fld == SeisTrcInfo::BinIDCrl ||
	   fld == SeisTrcInfo::SeqNr;
}



void SeisTrcBufDataPack::getAuxInfo( int itrc, int isamp, IOPar& iop ) const
{
    const SeisTrcBuf& buf = trcBuf();
    if ( buf.isEmpty() )
	return;

    FlatDataPack::getAuxInfo( itrc, isamp, iop );
    if ( itrc >= buf.size() ) itrc = buf.size() - 1;
    if ( itrc < 0 ) itrc = 0;
    const SeisTrcInfo& trcinfo = buf.get( itrc )->info();
    if ( mIsUdf(trcinfo.pick_) )
	iop.removeWithKey( SeisTrcInfo::toString(SeisTrcInfo::Pick) );
    else
	iop.set( SeisTrcInfo::toString(SeisTrcInfo::Pick), trcinfo.pick_ );

    if ( mIsUdf(trcinfo.offset_) )
	iop.removeWithKey( sKey::Offset() );
    else
    {
	iop.set( sKey::Offset(), trcinfo.offset_ );
	Seis::setGatherOffsetType( offsetType(), iop );
    }

    if ( mIsUdf(trcinfo.azimuth_) )
	iop.removeWithKey( sKey::Azimuth() );
    else
    {
	iop.set( sKey::Azimuth(), trcinfo.azimuth_ );
	Seis::setGatherAzimuthType( azimuthAngleType(), iop );
    }
}


TrcKey SeisTrcBufDataPack::getTrcKey( int itrc, int /* isamp */ ) const
{
    const SeisTrcBuf& buf = trcBuf();
    if ( buf.isEmpty() )
	return TrcKey::udf();

    if ( itrc >= buf.size() ) itrc = buf.size() - 1;
    if ( itrc < 0 ) itrc = 0;
    const SeisTrc* trc = buf.get( itrc );
    return trc->info().trcKey();
}


Coord3 SeisTrcBufDataPack::getCoord( int itrc, int isamp ) const
{
    const SeisTrcBuf& buf = trcBuf();
    if ( buf.isEmpty() )
	return Coord3::udf();

    if ( itrc >= buf.size() ) itrc = buf.size() - 1;
    if ( itrc < 0 ) itrc = 0;
    const SeisTrc* trc = buf.get( itrc );
	// Not calling getZ to avoid double work
    return Coord3( trc->info().coord_, trc->info().samplePos(isamp) );
}


double SeisTrcBufDataPack::getZ( int itrc, int isamp ) const
{
    const SeisTrcBuf& buf = trcBuf();
    if ( buf.isEmpty() )
	return mUdf(double);

    if ( itrc >= buf.size() ) itrc = buf.size() - 1;
    if ( itrc < 0 ) itrc = 0;
    const SeisTrc* trc = buf.get( itrc );
    return trc->info().samplePos( isamp );
}


bool SeisTrcBufDataPack::getTrcKeyZSampling( TrcKeyZSampling& tkzs ) const
{
    const SeisTrcBuf& buf = trcBuf();
    if ( buf.isEmpty() )
	return false;

    const SeisTrcInfo& seisinfo = buf.first()->info();
    const bool is3d = seisinfo.is3D();

    TrcKeySampling& tks = tkzs.hsamp_;
    tks.survid_ = seisinfo.geomSystem();

    tks.start_.inl() = tks.stop_.inl() = seisinfo.inl();
    tks.start_.crl() = tks.stop_.crl() = seisinfo.crl();
    tks.step_.inl() = is3d ? SI().inlStep() : 0;
    if ( is3d || buf.size() == 1 )
	tks.step_.crl() = SI().crlStep();
    else
	tks.step_.trcNr() = buf.get(1)->info().trcNr() - seisinfo.trcNr();

    for ( int idx=1; idx<buf.size(); idx++ )
	tks.include( buf.get(idx)->info().trcKey() );

    tkzs.zsamp_.setFrom( posData().range(false) );

    return true;
}


Seis::OffsetType SeisTrcBufDataPack::offsetType() const
{
    return offsettype_;
}


OD::AngleType SeisTrcBufDataPack::azimuthAngleType() const
{
    return azimuthangletype_;
}


uiString SeisTrcBufDataPack::dimName( bool dim0 ) const
{
    if ( !dim0 )
	return zDomain().userName();

    if ( posfld_ == SeisTrcInfo::TrcNr )
	return uiStrings::sTraceNumber();
    else if ( posfld_ == SeisTrcInfo::RefNr )
	return uiStrings::sSPNumber();
    else if ( posfld_ == SeisTrcInfo::Offset )
    {
	return Seis::isOffsetDist(offsetType()) ? uiStrings::sOffset()
						: uiStrings::sAngle();
    }
    else if ( posfld_ == SeisTrcInfo::Azimuth )
	return uiStrings::sAzimuth();
    else
	return SeisTrcInfo::toUiString( posfld_ );
}


uiString SeisTrcBufDataPack::dimUnitLbl( bool dim0, bool display,
					 bool abbreviated,
					 bool withparentheses ) const
{
    if ( !dim0 )
	return zDomain(display).uiUnitStr( withparentheses );

    if ( posfld_ == SeisTrcInfo::Offset || posfld_ == SeisTrcInfo::Azimuth )
    {
	const UnitOfMeasure* uom = dimUnit( dim0, display );
	return uom ? uom->getUiLabel( abbreviated, withparentheses )
		   : uiString::empty();
    }

    return SeisTrcInfo::getUnitLbl( posfld_, display,
				    abbreviated, withparentheses );
}


const UnitOfMeasure* SeisTrcBufDataPack::dimUnit( bool dim0, bool display) const
{
    if ( !dim0 )
	return zUnit( display );

    if ( posfld_ == SeisTrcInfo::Offset )
	return UnitOfMeasure::offsetUnit( offsetType() );
    else if ( posfld_ == SeisTrcInfo::Azimuth )
	return UnitOfMeasure::angleUnit( azimuthAngleType() );

    return SeisTrcInfo::getUnit( posfld_, display );
}


// SeisBufReader

SeisBufReader::SeisBufReader( SeisTrcReader& rdr, SeisTrcBuf& buf )
    : Executor("Collecting traces")
    , rdr_(rdr)
    , buf_(buf)
    , totnr_(-1)
    , msg_(tr("Reading traces"))
{
    if ( rdr.selData() && !rdr.selData()->isAll() )
	totnr_ = rdr.selData()->expectedNrTraces( rdr.is2D() );
}


SeisBufReader::~SeisBufReader()
{}


int SeisBufReader::nextStep()
{
    auto* newtrc = new SeisTrc;

    int res = rdr_.get( newtrc->info() );
    if ( res > 1 ) return MoreToDo();
    if ( res == 0 ) return Finished();

    if ( res<0 || !rdr_.get(*newtrc) )
    { msg_ = rdr_.errMsg(); return ErrorOccurred(); }

    buf_.add( newtrc );
    return MoreToDo();
}
