/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-1-1998
-*/


#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seispacketinfo.h"
#include "seisprovider.h"
#include "seisseldata.h"
#include "ptrman.h"
#include "sorting.h"
#include "flatposdata.h"
#include "staticstring.h"
#include "survinfo.h"
#include "iopar.h"
#include "keystrs.h"
#include "trckeyzsampling.h"
#include "bufstringset.h"
#include "arrayndimpl.h"
#include "od_iostream.h"


void SeisTrcBuf::deepErase()
{
    ::deepErase(trcs_);
}


int SeisTrcBuf::maxTrcSize() const
{
    return SeisTrcBuf::maxTrcSize( trcs_ );
}

int SeisTrcBuf::maxTrcSize( const TrcSet& tset )
{
    int sz = -1;
    for ( auto trc : tset )
    {
	const int trcsz = trc->size();
	if ( trcsz > sz )
	    sz = trcsz;
    }

    return sz;
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


void SeisTrcBuf::ensureCompatible( const TrcSet& ref )
{
    if ( &ref == &trcs_ )
	return;

    const int refsz = ref.size();
    if ( size() != refsz )
    {
	erase(); setIsOwner( true );
	for ( auto trc : ref )
	    add( new SeisTrc( *trc ) );
	return;
    }

    for ( int idx=0; idx<refsz; idx++ )
    {
	const SeisTrc& trcin = *ref.get( idx );
	SeisTrc& trcout = *get( idx );
	if ( trcout.size() != trcin.size() )
	    trcout.reSize( trcin.size(), false );
	if ( trcout.nrComponents() != trcin.nrComponents() )
	    trcout.setNrComponents( trcin.nrComponents() );
	const DataCharacteristics dc =
			    trcin.data().getInterpreter()->dataChar();
	if ( trcout.data().getInterpreter()->dataChar() != dc )
	    trcout.data().convertTo( dc );

	trcout.info() = trcin.info();
    }
}


void SeisTrcBuf::fill( SeisPacketInfo& spi ) const
{
    const int sz = size();
    if ( sz < 1 ) return;
    const SeisTrc* trc = first();
    BinID bid = trc->info().binID();
    const BinID pbid = bid;
    spi.inlrg.set( mUdf(int), -mUdf(int), 1 );
    spi.crlrg.set( mUdf(int), -mUdf(int), 1 );
    spi.zrg.set( mUdf(float), -mUdf(float), trc->info().sampling_.step );

    bool doneinl = false, donecrl = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	trc = get( idx ); bid = trc->info().binID();
	spi.inlrg.include( bid.inl(), false );
	spi.crlrg.include( bid.crl(), false);
	const SamplingData<float> trcsd = trc->info().sampling_;
	if ( !mIsUdf(trcsd.start) && !mIsUdf(trcsd.step) &&
	     !mIsZero(trcsd.step,Seis::cDefZEps()) )
	{
	    StepInterval<float> zrg(trcsd.start, trcsd.atIndex(trc->size()-1),
				    trcsd.step );
	    spi.zrg.include( zrg, false );
	}

	if ( !doneinl && bid.inl() != pbid.inl() )
	    { spi.inlrg.step = bid.inl() - pbid.inl(); doneinl = true; }
	if ( !donecrl && bid.crl() != pbid.crl() )
	    { spi.crlrg.step = bid.crl() - pbid.crl(); donecrl = true; }
    }

    if ( spi.inlrg.step < 0 ) spi.inlrg.step = -spi.inlrg.step;
    if ( spi.crlrg.step < 0 ) spi.crlrg.step = -spi.crlrg.step;
}


void SeisTrcBuf::add( SeisTrcBuf& tb )
{
    for ( int idx=0; idx<tb.size(); idx++ )
    {
	SeisTrc* trc = tb.get( idx );
	add( owner_ && trc ? new SeisTrc(*trc) : trc );
    }
}


ZGate SeisTrcBuf::zRange() const
{
    auto zrg = ZGate( mUdf(float), mUdf(float) );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( idx )
	    zrg.include( get(idx)->zRange() );
	else
	    zrg = get(idx)->zRange();
    }
    return zrg;
}


ZGate SeisTrcBuf::getZGate4Shifts( const ZValueSet& zvals, bool upw ) const
{
    const auto myzrg = zRange();
    if ( mIsUdf(myzrg.start) )
	return myzrg;

    auto zrg = ZGate( mUdf(float), mUdf(float) );
    for ( auto zval : zvals )
    {
	if ( mIsUdf(zval) )
	    continue;

	const float dz = upw ? -zval : zval;

	const Interval<float> curzrg( myzrg.start + dz, myzrg.stop + dz );
	if ( mIsUdf(zrg.start) )
	    zrg = curzrg;
	else
	{
	    zrg.include( curzrg.start, false );
	    zrg.include( curzrg.stop, false );
	}
    }

    return zrg;
}


void SeisTrcBuf::getShifted( ZGate zrg, const ZValueSet& zvals,
			     bool upward, float udfval, SeisTrcBuf& out ) const
{
    const auto nrtrcs = size();
    if ( nrtrcs < 1 || mIsUdf(zrg.start) || mIsUdf(zrg.stop) )
	return;

    StepInterval<float> newzrg( zrg );
    newzrg.step = get( 0 )->zStep();
    const auto newnrsamps = newzrg.nrSteps() + 1;
    const float zrest = newnrsamps*newzrg.step - zrg.width();
    newzrg.start -= zrest * 0.5f;

    for ( int itrc=0; itrc<nrtrcs; itrc++ )
    {
	const auto& inptrc = *get( itrc );
	const auto nrcomps = inptrc.nrComponents();
	auto* newtrc = new SeisTrc( newnrsamps );
	newtrc->setNrComponents( nrcomps );

	newtrc->info() = inptrc.info();
	newtrc->info().sampling_.start = newzrg.start;
	newtrc->info().sampling_.step = newzrg.step;
	newtrc->setAll( udfval );

	out.add( newtrc );
	if ( itrc >= zvals.size() )
	    continue;

	const auto zshift = zvals.get( itrc );
	const auto inptrcsz = inptrc.size();
	for ( int icomp=0; icomp<nrcomps; icomp++ )
	{
	    for ( int isamp=0; isamp<newnrsamps; isamp++ )
	    {
		const auto trcz = newtrc->zPos( isamp );
		const auto orgz = upward ? trcz + zshift : trcz - zshift;
		const int inpisamp = inptrc.nearestSample( orgz );
		if ( inpisamp >= 0 || inpisamp < inptrcsz )
		    newtrc->set( isamp, inptrc.getValue(orgz,icomp), icomp );
	    }
	}
    }
}


void SeisTrcBuf::addTrcsFrom( TrcSet& trcs )
{
    for ( auto trc : trcs )
	add( owner_ && trc ? new SeisTrc(*trc) : trc );
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
    sort_coupled( (float*)vals, (int*)idxs, sz );
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
	const bool issameinl = trc->info().lineNr()
				== singinlbuf.get(0)->info().lineNr();
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


int SeisTrcBuf::find( const BinID& binid ) const
{
    return doFind( binid );
}


int SeisTrcBuf::find( const Bin2D& bin2d ) const
{
    return doFind( IdxPair(bin2d.lineNr(),bin2d.trcNr()) );
}


int SeisTrcBuf::findTrcNr( int trcnr ) const
{
    return doFind( IdxPair(-999,trcnr) );
}


int SeisTrcBuf::doFind( const IdxPair& idxpair ) const
{
    int sz = size();
    int startidx = probableIdx( idxpair );
    int idx = startidx, pos = 0;
    const bool checklnr = idxpair.lineNr() != -999;
    while ( idx<sz && idx>=0 )
    {
	if ( checklnr )
	{
	    if ( get(idx)->info().idxPair() == idxpair )
		return idx;
	}
	else if ( get(idx)->info().trcNr() == idxpair.trcNr() )
	    return idx;

	if ( pos < 0 )
	    pos = -pos;
	else
	    pos = -pos-1;
	idx = startidx + pos;
	if ( idx < 0 )
	    { pos = -pos; idx = startidx + pos; }
	else if ( idx >= sz )
	    { pos = -pos-1; idx = startidx + pos; }
    }
    return -1;
}


int SeisTrcBuf::find( const SeisTrc* trc ) const
{
    if ( !trc )
	return -1;
    return trc->info().is2D() ? find( trc->info().bin2D() )
			      : find( trc->info().binID() );
}


int SeisTrcBuf::probableIdx( const IdxPair& ip ) const
{
    int sz = size(); if ( sz < 2 ) return 0;
    const auto start = trcs_[0]->info().idxPair();
    const auto stop = trcs_[sz-1]->info().idxPair();
    const IdxPair dist( start.inl()-stop.inl(), start.crl()-stop.crl() );
    if ( !dist.inl() && !dist.crl() )
	return 0;

    auto n1  = dist.inl() ? start.inl() : start.crl();
    auto n2  = dist.inl() ? stop.inl()  : stop.crl();
    auto pos = dist.inl() ? ip.inl()    : ip.crl();

    float fidx = ((sz-1.f) * (pos - n1)) / (n2-n1);
    int idx = mNINT32(fidx);
    if ( idx < 0 )
	idx = 0;
    if ( idx >= sz )
	idx = sz-1;
    return idx;
}


bool SeisTrcBuf::dump( const char* fnm, bool is2d, bool isps, int icomp ) const
{
    if ( isEmpty() ) return false;

    od_ostream strm( fnm );
    if ( strm.isBad() )
        return false;

    const SeisTrc& trc0 = *first();
    strm << trc0.info().sampling_.start
	 << ' ' << trc0.info().sampling_.step * SI().zDomain().userFactor()
	 << ' ' << trc0.size();

    for ( int itrc=0; itrc<size(); itrc++ )
    {
	strm << od_newline;
	const SeisTrc& trc = *get( itrc );
	if ( !is2d )
	    strm << trc.info().lineNr() << ' ' << trc.info().trcNr();
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


void SeisTrcBuf::ensureCompatible( const TrcSet& trcsin, TrcSet& trcsout )
{
    SeisTrcBuf bufout( trcsout );
    bufout.ensureCompatible( trcsin );
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

ArrayNDInfo* clone() const
{
    return new SeisTrcBufArray2DInfo(buf_);
}

size_type getSize( dim_idx_type dim ) const
{
    if ( dim == 0 )
	return buf_.size();

    const SeisTrc* trc = buf_.first();
    return trc ? trc->size() : 0;
}

    const SeisTrcBuf&	buf_;

};


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


void SeisTrcBufArray2D::getAuxInfo( Seis::GeomType gt, int itrc,
				    IOPar& iop ) const
{
    SeisTrc* trc = buf_->get( itrc );
    if ( trc )
	trc->info().getInterestingFlds( gt, iop );
}


SeisTrcBufDataPack::SeisTrcBufDataPack( const char* cat )
    : FlatDataPack( cat )
    , gt_(Seis::Line)
    , posfld_(SeisTrcInfo::TrcNr)
{
    arr2d_ = new SeisTrcBufArray2D( 0, true, 0 );
}


SeisTrcBufDataPack::SeisTrcBufDataPack( SeisTrcBuf* tbuf,
					Seis::GeomType gt,
					SeisTrcInfo::Fld fld,
					const char* cat, int icomp )
    : FlatDataPack( cat )
{
    setBuffer( tbuf, gt, fld, icomp, true );
}


SeisTrcBufDataPack::SeisTrcBufDataPack( const SeisTrcBuf& tbuf,
					Seis::GeomType gt,
					SeisTrcInfo::Fld fld,
					const char* cat, int icomp )
    : FlatDataPack( cat )
{
    setBuffer( const_cast<SeisTrcBuf*>(&tbuf), gt, fld, icomp, false );
}


SeisTrcBufDataPack::SeisTrcBufDataPack( const SeisTrcBufDataPack& b )
    : FlatDataPack( b.category(), 0 )
{
    const bool bufisours = b.arr2d_
			&& ((SeisTrcBufArray2D*)b.arr2d_)->bufIsMine();
    SeisTrcBuf* buf = const_cast<SeisTrcBuf*>( &b.trcBuf() );
    if ( buf && bufisours )
	buf = buf->clone();
    setBuffer( buf, b.gt_, b.posfld_, b.trcBufArr2D().getComp(), bufisours );
    setName( b.name() );
}


void SeisTrcBufDataPack::setBuffer( SeisTrcBuf* tbuf, Seis::GeomType gt,
				    SeisTrcInfo::Fld fld, int icomp, bool mine )
{
    posfld_ = fld;
    gt_ = gt;

    delete arr2d_;
    arr2d_ = new SeisTrcBufArray2D( tbuf, mine, icomp );

    if ( tbuf )
    {
	SeisTrcInfo::getAxisCandidates( gt_, flds_ );
	double ofv; float* hdrvals = tbuf->getHdrVals( posfld_, ofv );
	FlatPosData& pd = posData();
	const int tbufsz = tbuf ? tbuf->size() : 0;
	pd.setX1Pos( hdrvals, tbufsz, ofv );
	SeisPacketInfo pinf; tbuf->fill( pinf );
	StepInterval<double> zrg; assign( zrg, pinf.zrg );
	pd.setRange( false, zrg );
    }
}


bool SeisTrcBufDataPack::dimValuesInInt( const char* keystr ) const
{
    FixedString key( keystr );
    return key == SeisTrcInfo::toString(SeisTrcInfo::TrcNr) ||
	   key == SeisTrcInfo::toString(SeisTrcInfo::BinIDInl) ||
	   key == SeisTrcInfo::toString(SeisTrcInfo::BinIDCrl);
}


void SeisTrcBufDataPack::getAltDim0Keys( BufferStringSet& bss ) const
{
    for ( int idx=0; idx<flds_.size(); idx++ )
	bss.add( SeisTrcInfo::toString(flds_[idx]) );
}



double SeisTrcBufDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    const SeisTrcBuf& buf = trcBuf();
    return i0 < 0 || i0 >= buf.size() || ikey >= flds_.size()
	 ? FlatDataPack::getAltDim0Value( ikey, i0 )
	 : buf.get(i0)->info().getValue( flds_[ikey] );
}


void SeisTrcBufDataPack::getAuxInfo( int itrc, int isamp, IOPar& iop ) const
{
    const SeisTrcBuf& buf = trcBuf();
    const int bufsz = buf.size();
    if ( itrc >= bufsz || itrc < 0 ) return;

    const SeisTrc* trc = buf.get( itrc );
    trc->info().getInterestingFlds( gt_, iop );

    float z = trc->info().samplePos(isamp);
    if ( SI().zIsTime() ) z *= 1000;
    int z100 = mNINT32(z*100); z = mCast( float, z100 / 100 );
    iop.set( sKey::Z(), z );
}


Coord3 SeisTrcBufDataPack::getCoord( int itrc, int isamp ) const
{
    const SeisTrcBuf& buf = trcBuf();
    if ( buf.isEmpty() ) return Coord3();
    if ( itrc >= buf.size() ) itrc = buf.size() - 1;
    if ( itrc < 0 ) itrc = 0;
    const SeisTrc* trc = buf.get( itrc );
    return Coord3( trc->info().coord_, trc->info().samplePos(isamp) );
}


bool SeisTrcBufDataPack::getTrcKeyZSampling( TrcKeyZSampling& cs ) const
{
    const SeisTrcBuf& buf = trcBuf();
    if ( buf.isEmpty() )
	return false;

    cs.hsamp_.start_.inl() = cs.hsamp_.stop_.inl() =
	buf.first()->info().lineNr();
    cs.hsamp_.start_.crl() = cs.hsamp_.stop_.crl() =
	buf.first()->info().trcNr();
    cs.hsamp_.step_.inl() = SI().inlStep();
    cs.hsamp_.step_.crl() = SI().crlStep();

    for ( int idx=1; idx<buf.size(); idx++ )
	cs.hsamp_.include( buf.get( idx )->info().binID() );

    cs.zsamp_.setFrom( posData().range(false) );

    return true;
}


const char* SeisTrcBufDataPack::dimName( bool dim0 ) const
{
    mDeclStaticString( ret );
    if ( dim0 )
	ret.set( SeisTrcInfo::toString(posfld_) );
    else
	ret.set( toString( SI().zDomain().userName() ) );
    return ret.buf();
}


void SeisTrcBufDataPack::doDumpInfo( IOPar& iop ) const
{
    FlatDataPack::doDumpInfo( iop );

    Seis::putInPar( gt_, iop );
    iop.set( "Selected Data", SeisTrcInfo::toString(posfld_) );
}


SeisBufReader::SeisBufReader( Seis::Provider& prov, SeisTrcBuf& buf )
    : Executor("Collecting traces")
    , prov_(prov)
    , buf_(buf)
    , totnr_(prov.totalNr())
    , msg_(tr("Reading traces"))
{}


int SeisBufReader::nextStep()
{
    SeisTrc* newtrc = new SeisTrc;

    const uiRetVal uirv = prov_.getNext( *newtrc );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    return Executor::Finished();

	msg_ = uirv;
	return Executor::ErrorOccurred();
    }

    buf_.add( newtrc );
    return Executor::MoreToDo();
}
