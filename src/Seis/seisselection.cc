/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data keys
-*/

static const char* rcsID = "$Id: seisselection.cc,v 1.6 2007-12-04 12:25:06 cvsbert Exp $";

#include "seisselectionimpl.h"
#include "cubesampling.h"
#include "pickset.h"
#include "picksettr.h"
#include "binidvalset.h"
#include "polygon.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "keystrs.h"
#include "linekey.h"
#include "strmprov.h"


Seis::SelData::SelData()
    : linekey_(*new LineKey)
    , isall_(false)
{
}


Seis::SelData::~SelData()
{
    delete &linekey_;
}


void Seis::SelData::copyFrom( const Seis::SelData& sd )
{
    isall_ = sd.isall_;
    linekey_ = sd.linekey_;
}


void Seis::SelData::removeFromPar( IOPar& iop )
{
    iop.set( sKey::BinIDSel, sKey::No );
}


void Seis::SelData::extendH( const BinID& so, const BinID* stepoutstep )
{
    BinID sos( SI().inlStep(), SI().crlStep() );
    if ( stepoutstep ) sos = *stepoutstep;
    doExtendH( so, sos );
}


Seis::SelData* Seis::SelData::get( Type t )
{
    switch ( t )
    {
    case Seis::Table:	return new Seis::TableSelData;
    case Seis::Polygon:	return new Seis::PolySelData;
    default:		return new Seis::RangeSelData( true );
    }
}


Seis::SelData* Seis::SelData::get( const IOPar& iop )
{
    Type t = Seis::Range;
    const char* res = iop.find( sKey::BinIDSel );
    if ( res && *res )
	t = Seis::selTypeOf(res);

    Seis::SelData* sd = get( t );
    sd->usePar( iop );
    return sd;
}


Interval<int> Seis::SelData::inlRange() const
{
    return HorSampling(true).inlRange();
}


Interval<int> Seis::SelData::crlRange() const
{
    return HorSampling(true).crlRange();
}


Interval<float> Seis::SelData::zRange() const
{
    Interval<float> ret; assign( ret, SI().zRange(false) );
    return ret;
}



void Seis::SelData::fillPar( IOPar& iop ) const
{
    iop.set( sKey::BinIDSel, isall_ ? sKey::No : Seis::nameOf(type()) );
    if ( linekey_.isEmpty() )
	iop.removeWithKey( sKey::LineKey );
    else
	iop.set( sKey::LineKey, linekey_ );
}


void Seis::SelData::usePar( const IOPar& iop )
{
    iop.get( sKey::LineKey, linekey_ );
    const char* res = iop.find( sKey::Attribute );
    if ( res && *res )
	linekey_.setAttrName( res );
}


int Seis::SelData::tracesInSI() const
{
    return HorSampling(true).totalNr();
}


//--- Range ---


Seis::RangeSelData::RangeSelData( bool initsi )
    : cs_(*new CubeSampling(initsi))
{
}


Seis::RangeSelData::RangeSelData( const HorSampling& hs )
    : cs_(*new CubeSampling(false))
{
    cs_.hrg = hs;
    cs_.zrg = SI().zRange(false);
}


Seis::RangeSelData::RangeSelData( const CubeSampling& cs )
    : cs_(*new CubeSampling(cs))
{
}


Seis::RangeSelData::RangeSelData( const Seis::RangeSelData& sd )
    : cs_(*new CubeSampling(false))
{
    copyFrom(sd);
}


Seis::RangeSelData::~RangeSelData()
{
    delete &cs_;
}


void Seis::RangeSelData::copyFrom( const Seis::SelData& sd )
{
    if ( this == &sd ) return;

    SelData::copyFrom( sd );
    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::RangeSelData&,rsd,sd)
	cs_ = rsd.cs_;
    }
    else
    {
	Interval<int> rg( sd.inlRange() );
	cs_.hrg.start.inl = rg.start; cs_.hrg.stop.inl = rg.stop;
	rg = sd.crlRange();
	cs_.hrg.start.crl = rg.start; cs_.hrg.stop.crl = rg.stop;
	assign( cs_.zrg, sd.zRange() );
    }
}


Interval<int> Seis::RangeSelData::inlRange() const
{
    return isall_ ? Seis::SelData::inlRange() : cs_.hrg.inlRange();
}


Interval<int> Seis::RangeSelData::crlRange() const
{
    return isall_ ? Seis::SelData::crlRange() : cs_.hrg.crlRange();
}


Interval<float> Seis::RangeSelData::zRange() const
{
    return isall_ ? Seis::SelData::zRange() : cs_.zrg;
}


bool Seis::RangeSelData::setInlRange( Interval<int> rg )
{
    cs_.hrg.start.inl = rg.start; cs_.hrg.stop.inl = rg.stop;
    return true;
}


bool Seis::RangeSelData::setCrlRange( Interval<int> rg )
{
    cs_.hrg.start.crl = rg.start; cs_.hrg.stop.crl = rg.stop;
    return true;
}


bool Seis::RangeSelData::setZRange( Interval<float> rg )
{
    cs_.zrg.start = rg.start;
    cs_.zrg.stop = rg.stop;
    return true;
}


void Seis::RangeSelData::fillPar( IOPar& iop ) const
{
    Seis::SelData::fillPar( iop );
    if ( !isall_ )
	cs_.fillPar( iop );
}


void Seis::RangeSelData::usePar( const IOPar& iop )
{
    Seis::SelData::usePar( iop );
    if ( !isall_ )
	cs_.usePar( iop );
}


void Seis::RangeSelData::extendZ( const Interval<float>& zrg )
{
    cs_.zrg.start += zrg.start;
    cs_.zrg.stop += zrg.stop;
}


void Seis::RangeSelData::doExtendH( BinID so, BinID sos )
{
    cs_.hrg.start.inl -= so.inl * sos.inl;
    cs_.hrg.start.crl -= so.crl * sos.crl;
    cs_.hrg.stop.inl += so.inl * sos.inl;
    cs_.hrg.stop.crl += so.crl * sos.crl;
}


void Seis::RangeSelData::include( const Seis::SelData& sd )
{
    if ( isall_ )
	return;
    if ( sd.isAll() )
	{ isall_ = true; return; }

    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::RangeSelData&,rsd,sd)
	cs_.include( rsd.cs_ );
	return;
    }

    Interval<int> rg( sd.inlRange() );
    if ( cs_.hrg.start.inl > rg.start ) cs_.hrg.start.inl = rg.start;
    if ( cs_.hrg.stop.inl < rg.stop ) cs_.hrg.stop.inl = rg.stop;
    rg = sd.crlRange();
    if ( cs_.hrg.start.crl > rg.start ) cs_.hrg.start.crl = rg.start;
    if ( cs_.hrg.stop.crl < rg.stop ) cs_.hrg.stop.crl = rg.stop;
    const Interval<float> zrg( sd.zRange() );
    if ( cs_.zrg.start > rg.start ) cs_.zrg.start = rg.start;
    if ( cs_.zrg.stop < rg.stop ) cs_.zrg.stop = rg.stop;
}


int Seis::RangeSelData::selRes( const BinID& bid ) const
{
    if ( isall_ ) return true;

    int inlres = cs_.hrg.start.inl > bid.inl || cs_.hrg.stop.inl < bid.inl
		? 2 : 0;
    int crlres = cs_.hrg.start.crl > bid.crl || cs_.hrg.stop.crl < bid.crl
		? 2 : 0;
    return inlres + 256 * crlres;
}


int Seis::RangeSelData::expectedNrTraces( bool for2d, const BinID* step ) const
{
    if ( isall_ ) return tracesInSI();

    HorSampling hs( cs_.hrg );
    if ( step ) hs.step = *step;
    const int nrinl = for2d ? 1 : hs.nrInl();
    const int nrcrl = hs.nrCrl();
    return nrinl * nrcrl;
}


//--- Table ---


Seis::TableSelData::TableSelData()
    : bvs_(*new BinIDValueSet(1,false))
    , extraz_(0,0)
{
}


Seis::TableSelData::TableSelData( const BinIDValueSet& bvs,
				  const Interval<float>* extraz )
    : bvs_(*new BinIDValueSet(bvs))
    , extraz_(0,0)
{
    bvs_.setNrVals( 1, true );
    if ( extraz ) extraz_ = *extraz;
}


Seis::TableSelData::TableSelData( const Seis::TableSelData& sd )
    : bvs_(*new BinIDValueSet(1,false))
    , extraz_(0,0)
{
    copyFrom(sd);
}


Seis::TableSelData::~TableSelData()
{
    delete &bvs_;
}


void Seis::TableSelData::copyFrom( const Seis::SelData& sd )
{
    if ( this == &sd ) return;

    SelData::copyFrom( sd );
    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::TableSelData&,tsd,sd)
	bvs_ = tsd.bvs_;
	extraz_ = tsd.extraz_;
    }
    else
    {
	pErrMsg( "Not impl" );
    }
}


Interval<int> Seis::TableSelData::inlRange() const
{
    if ( isall_  ) return Seis::SelData::inlRange();
    return bvs_.inlRange();
}


Interval<int> Seis::TableSelData::crlRange() const
{
    if ( isall_  ) return Seis::SelData::crlRange();
    return bvs_.crlRange();
}


Interval<float> Seis::TableSelData::zRange() const
{
    if ( isall_ ) return Seis::SelData::zRange();
    return bvs_.valRange( 0 ) + extraz_;
}


void Seis::TableSelData::fillPar( IOPar& iop ) const
{
    Seis::SelData::fillPar( iop );
    if ( isall_ ) return;
    iop.set( "Table.ExtraZ", extraz_.start, extraz_.stop );
    bvs_.fillPar( iop, "Table.Data" );
}


void Seis::TableSelData::usePar( const IOPar& iop )
{
    Seis::SelData::usePar( iop );
    if ( isall_ ) { bvs_.empty(); return; }

    const char* res = iop.find( sKey::FileName );
    if ( !res || !*res )
	bvs_.usePar( iop, "Table.Data" );
    else
    {
	StreamData sd( StreamProvider(res).makeIStream() );
	if ( sd.usable() )
	    bvs_.getFrom( *sd.istrm );
    }
    iop.get( "Table.ExtraZ", extraz_.start, extraz_.stop );
}


void Seis::TableSelData::extendZ( const Interval<float>& zrg )
{
    extraz_.start += zrg.start;
    extraz_.stop += zrg.stop;
}


void Seis::TableSelData::doExtendH( BinID so, BinID sos )
{
    bvs_.extend( so, sos );
}


void Seis::TableSelData::include( const Seis::SelData& sd )
{
    if ( isall_ )
	return;
    if ( sd.isAll() )
	{ isall_ = true; return; }

    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::TableSelData&,tsd,sd)
	bvs_.append( tsd.bvs_ );
	if ( extraz_.start < tsd.extraz_.start )
	    extraz_.start = tsd.extraz_.start;
	if ( extraz_.stop > tsd.extraz_.stop )
	    extraz_.stop = tsd.extraz_.stop;
    }
    else
    {
	pErrMsg( "Not impl" );
    }
}


int Seis::TableSelData::selRes( const BinID& bid ) const
{
    if ( isall_ ) return true;

    const BinIDValueSet::Pos pos( bvs_.findFirst(bid) );
    if ( pos.j >= 0 ) return 0;

    const int inlres = pos.i < 0 ? 2 : 0;
    const int crlres = 1; // Maybe not true, but safe
    return inlres + 256 * crlres;
}


int Seis::TableSelData::expectedNrTraces( bool for2d, const BinID* step ) const
{
    return isall_ ? tracesInSI() : bvs_.totalSize();
}


//--- Poly ---


Seis::PolySelData::PolySelData()
    : poly_(*new ODPolygon<float>)
{
    initZrg( 0 );
}


Seis::PolySelData::PolySelData( const ODPolygon<float>& poly,
			        const Interval<float>* zrg )
    : poly_(*new ODPolygon<float>(poly))
{
    initZrg( zrg );
}


Seis::PolySelData::PolySelData( const ODPolygon<int>& poly,
			        const Interval<float>* zrg )
    : poly_(*new ODPolygon<float>)
{
    for ( int idx=0; idx<poly.size(); idx++ )
    {
	const Geom::Point2D<int>& pt = poly.getVertex( idx );
	poly_.add( Geom::Point2D<float>( pt.x, pt.y ) );
    }
    initZrg( zrg );
}


Seis::PolySelData::PolySelData( const Seis::PolySelData& sd )
    : poly_(*new ODPolygon<float>(sd.poly_))
    , zrg_(sd.zrg_)
{
}


void Seis::PolySelData::initZrg( const Interval<float>* zrg )
{
    if ( zrg )
	zrg_ = *zrg;
    else
	assign( zrg_, SI().zRange(false) );
}


Seis::PolySelData::~PolySelData()
{
    delete &poly_;
}


void Seis::PolySelData::copyFrom( const Seis::SelData& sd )
{
    if ( this == &sd ) return;

    SelData::copyFrom( sd );
    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::PolySelData&,psd,sd)
	poly_ = psd.poly_;
	zrg_ = psd.zrg_;
    }
    else
    {
	pErrMsg( "Not impl" );
    }
}


Interval<int> Seis::PolySelData::inlRange() const
{
    if ( isall_ ) return Seis::SelData::inlRange();
    Interval<float> rg( poly_.getRange(true) );
    return Interval<int>( mNINT(rg.start), mNINT(rg.stop) );
}


Interval<int> Seis::PolySelData::crlRange() const
{
    if ( isall_  ) return Seis::SelData::crlRange();
    Interval<float> rg( poly_.getRange(false) );
    return Interval<int>( mNINT(rg.start), mNINT(rg.stop) );
}


Interval<float> Seis::PolySelData::zRange() const
{
    if ( isall_ ) return Seis::SelData::zRange();
    return zrg_;
}


void Seis::PolySelData::fillPar( IOPar& iop ) const
{
    Seis::SelData::fillPar( iop );
    if ( isall_ ) return;
    iop.set( "Polygon.ZRange", zrg_.start, zrg_.stop );
    ::fillPar( iop, poly_, "Poly" );
}


void Seis::PolySelData::usePar( const IOPar& iop )
{
    Seis::SelData::usePar( iop );
    if ( isall_ ) { poly_.setEmpty(); return; }

    iop.get( "Polygon.ZRange", zrg_.start, zrg_.stop );
    const char* res = iop.find( sKey::Polygon );
    if ( !res || !*res )
	::usePar( iop, poly_, "Poly" );
    else
    {
	PtrMan<IOObj> ioobj = IOM().get( res );
	if ( !ioobj ) return;
	Pick::Set ps; BufferString msg;
	if ( !PickSetTranslator::retrieve(ps,ioobj,msg) )
	    { ErrMsg( msg ); return; }

	poly_.setEmpty();
	for ( int idx=0; idx<ps.size(); idx++ )
	{
	    const Pick::Location& pl = ps[idx];
	    Coord fbid = SI().binID2Coord().transformBackNoSnap( pl.pos );
	    poly_.add( Geom::Point2D<float>(fbid.x,fbid.y) );
	}
    }
}


void Seis::PolySelData::extendZ( const Interval<float>& zrg )
{
    zrg_.start += zrg.start;
    zrg_.stop += zrg.stop;
}


void Seis::PolySelData::doExtendH( BinID so, BinID sos )
{
    //TODO (this is a tough one)
    // poly_.extend( so, sos );
}


void Seis::PolySelData::include( const Seis::SelData& sd )
{
    if ( isall_ )
	return;
    if ( sd.isAll() )
	{ isall_ = true; return; }

    if ( sd.type() == type() )
    {
	mDynamicCastGet(const Seis::PolySelData&,psd,sd)
	if ( zrg_.start < psd.zrg_.start )
	    zrg_.start = psd.zrg_.start;
	if ( zrg_.stop > psd.zrg_.stop )
	    zrg_.stop = psd.zrg_.stop;
	//TODO (should be rather standard)
	// poly_.joinWith( psd.poly_ );
    }
    else
    {
	//TODO with Range it should be doable; with Table it can't be done
	pErrMsg( "Not impl" );
    }
}


int Seis::PolySelData::selRes( const BinID& bid ) const
{
    if ( isall_ ) return true;

    const Geom::Point2D<float> pt( bid.inl, bid.crl );
    if ( poly_.isInside(pt,true,1e-6) )
	return true;

    Interval<float> rg( poly_.getRange(true) );
    const int inlres = rg.includes(pt.x,1e-6) ? 0 : 2;
    const int crlres = 1; // Maybe not true, but safe
    return inlres + 256 * crlres;
}


int Seis::PolySelData::expectedNrTraces( bool for2d, const BinID* step ) const
{
    if ( isall_ || for2d ) return tracesInSI();

    //TODO better impl
    HorSampling hs; hs.set( inlRange(), crlRange() );
    if ( step ) hs.step = *step;
    return hs.nrInl() * hs.nrCrl();
}
