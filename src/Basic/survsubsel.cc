/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : somewhere around 1999
-*/


#include "survsubsel.h"
#include "cubesampling.h"
#include "cubesubsel.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "survgeom2d.h"
#include "survgeom3d.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uistrings.h"


const LineHorSubSel& LineHorSubSel::empty()
{ static const LineHorSubSel ret( pos_steprg_type(0,-1,1) ); return ret; }
LineHorSubSel& LineHorSubSel::dummy()
{ static LineHorSubSel ret( pos_steprg_type(0,0,1) ); return ret; }
const LineSubSel& LineSubSel::empty()
{ static const LineSubSel ret( pos_steprg_type(0,-1,1) ); return ret; }
LineSubSel& LineSubSel::dummy()
{ static LineSubSel ret( pos_steprg_type(0,0,1) ); return ret; }


mDefineEnumUtils(CubeSubSel,SliceType,"Slice Direction") {
    "Inline",
    "CrossLine",
    "ZSlice",
    0
};

template<>
void EnumDefImpl<CubeSubSel::SliceType>::init()
{
    uistrings_ += uiStrings::sInline();
    uistrings_ += uiStrings::sCrossline();
    uistrings_ += uiStrings::sZSlice();
}


bool Survey::SubSel::getInfo( const IOPar& iop, bool& is2d, GeomID& geomid )
{
    is2d = false;
    geomid = GeomID::get3D();

    int igs = (int)OD::VolBasedGeom;
    if ( !iop.get(sKey::GeomSystem(),igs) )
    {
	if ( !iop.get(sKey::SurveyID(),igs) )
	    return false;
	if ( igs < (int)OD::SynthGeom || igs > (int)OD::LineBasedGeom )
	    igs = (int)OD::VolBasedGeom;
    }

    is2d = igs == (int)OD::LineBasedGeom;
    if ( is2d )
	return iop.get( sKey::GeomID(), geomid )
	    && SurvGeom::isUsable( geomid );

    return true;
}


void Survey::SubSel::fillParInfo( IOPar& iop, bool is2d, GeomID gid )
{
    const OD::GeomSystem gs = is2d ? OD::LineBasedGeom : OD::VolBasedGeom;
    iop.set( sKey::GeomSystem(), (int)gs );
    if ( is2d )
	iop.set( sKey::GeomID(), gid );
    else
	iop.removeWithKey( sKey::GeomID() );
}


Survey::SubSel* Survey::HorSubSel::duplicate() const
{
    const auto* lhss = asLineHorSubSel();
    if ( lhss )
	return new LineHorSubSel( *lhss );
    else
	return new CubeHorSubSel( *asCubeHorSubSel() );
}


LineHorSubSel* Survey::HorSubSel::asLineHorSubSel()
{
    return mGetDynamicCast( LineHorSubSel*, this );
}


const LineHorSubSel* Survey::HorSubSel::asLineHorSubSel() const
{
    return mGetDynamicCast( const LineHorSubSel*, this );
}


CubeHorSubSel* Survey::HorSubSel::asCubeHorSubSel()
{
    return mGetDynamicCast( CubeHorSubSel*, this );
}


const CubeHorSubSel* Survey::HorSubSel::asCubeHorSubSel() const
{
    return mGetDynamicCast( const CubeHorSubSel*, this );
}


Survey::HorSubSel* Survey::HorSubSel::get( const TrcKeySampling& tks )
{
    if ( tks.is2D() )
	return new LineHorSubSel( tks );
    else
	return new CubeHorSubSel( tks );
}


Survey::HorSubSel* Survey::HorSubSel::create( const IOPar& iop )
{
    bool is2d; GeomID gid;
    if ( !getInfo(iop,is2d,gid) )
	return 0;

    HorSubSel* ret = 0;
    if ( is2d )
	ret = new LineHorSubSel( gid );
    else
	ret = new CubeHorSubSel;
    if ( ret )
	ret->usePar( iop );

    return ret;
}


bool Survey::HorSubSel::usePar( const IOPar& iop )
{
    return doUsePar( iop );
}


void Survey::HorSubSel::fillPar( IOPar& iop ) const
{
    fillParInfo( iop, is2D(), geomID() );
    doFillPar( iop );
}


Survey::GeomSubSel::GeomSubSel( const z_steprg_type& zrg )
    : zss_( zrg )
{
}


Survey::SubSel* Survey::GeomSubSel::duplicate() const
{
    const auto* lss = asLineSubSel();
    if ( lss )
	return lss->duplicate();
    else
	return asCubeSubSel()->duplicate();
}


LineSubSel* Survey::GeomSubSel::asLineSubSel()
{
    return mGetDynamicCast( LineSubSel*, this );
}


const LineSubSel* Survey::GeomSubSel::asLineSubSel() const
{
    return mGetDynamicCast( const LineSubSel*, this );
}


CubeSubSel* Survey::GeomSubSel::asCubeSubSel()
{
    return mGetDynamicCast( CubeSubSel*, this );
}


const CubeSubSel* Survey::GeomSubSel::asCubeSubSel() const
{
    return mGetDynamicCast( const CubeSubSel*, this );
}


Survey::GeomSubSel* Survey::GeomSubSel::get( GeomID gid )
{
    if ( gid.is2D() )
	return new LineSubSel( gid );
    else
	return new CubeSubSel;
}


Survey::GeomSubSel* Survey::GeomSubSel::get( const TrcKeyZSampling& tkzs )
{
    if ( tkzs.is2D() )
	return new LineSubSel( tkzs );
    else
	return new CubeSubSel( tkzs );
}


Survey::GeomSubSel* Survey::GeomSubSel::create( const IOPar& iop )
{
    bool is2d; GeomID gid;
    if ( !getInfo(iop,is2d,gid) )
	return 0;

    GeomSubSel* ret = 0;
    if ( is2d )
	ret = new LineSubSel( gid );
    else
	ret = new CubeSubSel;
    if ( ret )
	ret->usePar( iop );

    return ret;
}


bool Survey::GeomSubSel::usePar( const IOPar& iop )
{
    if ( !horSubSel().usePar(iop) )
	return false;
    zss_.usePar( iop );
    return true;
}


void Survey::GeomSubSel::fillPar( IOPar& iop ) const
{
    horSubSel().fillPar( iop );
    zss_.fillPar( iop );
}


LineHorSubSel::LineHorSubSel( GeomID gid )
    : LineHorSubSel( Geometry::get2D(gid) )
{
}


LineHorSubSel::LineHorSubSel( const Geometry2D& geom )
    : LineHorSubSel( geom.trcNrRange() )
{
    geomid_ = geom.geomID();
}


LineHorSubSel::LineHorSubSel( const pos_steprg_type& trcnrrg )
    : Pos::IdxSubSel1D( trcnrrg )
{
}


LineHorSubSel::LineHorSubSel( const Bin2D& b2d )
    : LineHorSubSel( b2d.geomID(), b2d.trcNr() )
{
}


LineHorSubSel::LineHorSubSel( GeomID gid, trcnr_type tnr )
    : LineHorSubSel(gid)
{
    auto trcnrrg = trcNrRange();
    trcnrrg.start = trcnrrg.stop = tnr;
    setTrcNrRange( trcnrrg );
}


LineHorSubSel::LineHorSubSel( const TrcKeySampling& tks )
    : Pos::IdxSubSel1D( tks.trcRange() )
    , geomid_(tks.getGeomID())
{
}


bool LineHorSubSel::includes( const LineHorSubSel& oth ) const
{
    const auto trcnrrg = trcNrRange();
    const auto othtrcnrrg = oth.trcNrRange();
    return trcnrrg.step == othtrcnrrg.step
	&& includes( othtrcnrrg.start )
	&& includes( othtrcnrrg.stop );
}


bool LineHorSubSel::includes( const Bin2D& b2d ) const
{
    return b2d.geomID() == geomid_ && includes( b2d.trcNr() );
}


void LineHorSubSel::merge( const LineHorSubSel& oth )
{
    if ( geomid_ != oth.geomid_ )
	{ pErrMsg("Probably error, geomids do not match"); }
    trcNrSubSel().widenTo( oth.trcNrSubSel() );
}


void LineHorSubSel::limitTo( const LineHorSubSel& oth )
{
    if ( geomid_ != oth.geomid_ )
	{ pErrMsg("Probably error, geomids do not match"); }
    trcNrSubSel().limitTo( oth.trcNrSubSel() );
}


bool LineHorSubSel::doUsePar( const IOPar& inpiop )
{
    const IOPar* iop = &inpiop;
    PtrMan<IOPar> pardestroyer;
    if ( !iop->isPresent(sKey::GeomID()) )
    {
	IOPar* subiop = inpiop.subselect( sKey::Line() );
	pardestroyer = subiop;
	iop = subiop;
    }

    auto geomid = geomid_;
    if ( !iop->get(sKey::GeomID(),geomid)
      || !SurvGeom::isPresent(geomid) )
	return false;

    auto trcrg( trcNrRange() );
    *this = LineHorSubSel( geomid ); // clear to no subselection
    iop->get( sKey::FirstTrc(), trcrg.start );
    iop->get( sKey::LastTrc(), trcrg.stop );
    if ( !iop->get(sKey::StepTrc(),trcrg.step) )
	iop->get( sKey::StepCrl(), trcrg.step );
    data_.setOutputPosRange( trcrg );

    return true;
}


void LineHorSubSel::doFillPar( IOPar& iop ) const
{
    const auto trcrg( trcNrRange() );
    iop.set( sKey::FirstTrc(), trcrg.start );
    iop.set( sKey::LastTrc(), trcrg.stop );
    iop.set( sKey::StepTrc(), trcrg.step );
}


LineHorSubSelSet::LineHorSubSelSet( GeomID gid )
{
    add( new LineHorSubSel(gid) );
}


LineHorSubSelSet::LineHorSubSelSet( GeomID gid, trcnr_type tnr )
{
    add( new LineHorSubSel(gid,tnr) );
}


LineHorSubSelSet::totalsz_type LineHorSubSelSet::totalSize() const
{
    totalsz_type ret = 0;
    for ( auto lhss : *this )
	ret += lhss->totalSize();
    return ret;
}


bool LineHorSubSelSet::hasAllLines() const
{
    GeomIDSet allgids;
    SurvGeom2D::getGeomIDs( allgids );
    for ( auto gid : allgids )
	if ( !find(gid) )
	    return false;
    return true;
}


bool LineHorSubSelSet::isAll() const
{
    return hasAllLines() && hasFullRange();
}


bool LineHorSubSelSet::hasFullRange() const
{
    for ( auto lhss : *this )
	if ( !lhss->hasFullRange() )
	    return false;
    return true;
}


void LineHorSubSelSet::merge( const LineHorSubSelSet& oth )
{
    for ( auto othlhss : oth )
    {
	auto* lhss = doFind( othlhss->geomID() );
	if ( lhss )
	    lhss->merge( *othlhss );
	else
	    add( new LineHorSubSel(*othlhss) );
    }
}


void LineHorSubSelSet::limitTo( const LineHorSubSelSet& oth )
{
    ObjectSet<LineHorSubSel> torem;

    for ( auto lhss : *this )
    {
	auto* othlhss = oth.doFind( lhss->geomID() );
	if ( othlhss )
	    lhss->trcNrSubSel().limitTo( othlhss->trcNrSubSel() );
	else
	    torem.add( lhss );
    }

    for ( auto lhss : torem )
	removeSingle( indexOf(lhss) );
}


void LineHorSubSelSet::addStepout( trcnr_type so )
{
    for ( auto lhss : *this )
	lhss->addStepout( so );
}


LineHorSubSel* LineHorSubSelSet::doFind( GeomID gid ) const
{
    for ( auto lhss : *this )
	if ( lhss->geomID() == gid )
	    return const_cast<LineHorSubSel*>( lhss );
    return 0;
}


CubeHorSubSel::CubeHorSubSel( OD::SurvLimitType slt )
    : CubeHorSubSel( Geometry::get3D(slt) )
{
}


CubeHorSubSel::CubeHorSubSel( const Geometry3D& geom )
    : CubeHorSubSel( geom.inlRange(), geom.crlRange() )
{
}


CubeHorSubSel::CubeHorSubSel( const pos_steprg_type& inlrg,
			      const pos_steprg_type& crlrg )
    : Pos::IdxSubSel2D( inlrg, crlrg )
{
}


CubeHorSubSel::CubeHorSubSel( const BinID& bid )
    : CubeHorSubSel()
{
    auto rg = inlRange(); rg.start = rg.stop = bid.inl();
    setInlRange( rg );
    rg = crlRange(); rg.start = rg.stop = bid.crl();
    setCrlRange( rg );
}


CubeHorSubSel::CubeHorSubSel( const HorSampling& hsamp )
    : CubeHorSubSel( hsamp.inlRange(), hsamp.crlRange() )
{
}


CubeHorSubSel::CubeHorSubSel( const TrcKeySampling& tks )
    : CubeHorSubSel( tks.lineRange(), tks.trcRange() )
{
}


Pos::GeomID CubeHorSubSel::geomID() const
{
    return GeomID::get3D();
}


CubeHorSubSel::totalsz_type CubeHorSubSel::totalSize() const
{
    return ((totalsz_type)nrInl()) * nrCrl();
}


bool CubeHorSubSel::includes( const CubeHorSubSel& oth ) const
{
    const auto inlrg = inlRange();
    const auto crlrg = crlRange();
    return includes( BinID(inlrg.start,crlrg.start) )
	&& includes( BinID(inlrg.start,crlrg.stop) )
	&& includes( BinID(inlrg.stop,crlrg.start) )
	&& includes( BinID(inlrg.stop,crlrg.stop) );
}


void CubeHorSubSel::merge( const CubeHorSubSel& oth )
{
    inlSubSel().widenTo( oth.inlSubSel() );
    crlSubSel().widenTo( oth.crlSubSel() );
}


void CubeHorSubSel::limitTo( const CubeHorSubSel& oth )
{
    inlSubSel().limitTo( oth.inlSubSel() );
    crlSubSel().limitTo( oth.crlSubSel() );
}


void CubeHorSubSel::addStepout( pos_type iso, pos_type cso )
{
    inlSubSel().addStepout( iso );
    crlSubSel().addStepout( cso );
}


bool CubeHorSubSel::doUsePar( const IOPar& iop )
{
    auto rg( inlRange() );
    if ( !iop.get(sKey::InlRange(),rg) )
    {
	iop.get( sKey::FirstInl(), rg.start );
	iop.get( sKey::LastInl(), rg.stop );
	iop.get( sKey::StepInl(), rg.step );
    }
    setInlRange( rg );
    rg = crlRange();
    if ( !iop.get(sKey::CrlRange(),rg) )
    {
	iop.get( sKey::FirstCrl(), rg.start );
	iop.get( sKey::LastCrl(), rg.stop );
	iop.get( sKey::StepCrl(), rg.step );
    }
    setCrlRange( rg );
    return true;
}


void CubeHorSubSel::doFillPar( IOPar& iop ) const
{
    const auto inlrg( inlRange() );
    iop.set( sKey::FirstInl(), inlrg.start );
    iop.set( sKey::LastInl(), inlrg.stop );
    iop.set( sKey::StepInl(), inlrg.step );
    const auto crlrg( crlRange() );
    iop.set( sKey::FirstCrl(), crlrg.start );
    iop.set( sKey::LastCrl(), crlrg.stop );
    iop.set( sKey::StepCrl(), crlrg.step );
}



LineSubSel::LineSubSel( GeomID gid )
    : LineSubSel( SurvGeom::get2D(gid) )
{
}


LineSubSel::LineSubSel( const Geometry2D& geom )
    : Survey::GeomSubSel( geom.zRange() )
    , hss_( geom )
{
    hss_.setGeomID( geom.geomID() );
}


LineSubSel::LineSubSel( const pos_steprg_type& trnrrg )
    : LineSubSel( trnrrg, SurvGeom::get3D().zRange() )
{
}


LineSubSel::LineSubSel( const pos_steprg_type& hrg, const z_steprg_type& zrg )
    : Survey::GeomSubSel( zrg )
    , hss_( hrg )
{
}


LineSubSel::LineSubSel( const pos_steprg_type& hrg, const ZSubSel& zss )
    : LineSubSel( hrg, zss.outputZRange() )
{
}


LineSubSel::LineSubSel( const LineHorSubSel& hss )
    : LineSubSel(hss.geomID())
{
    hss_ = hss;
}


LineSubSel::LineSubSel( GeomID gid, trcnr_type tnr )
    : LineSubSel(gid)
{
    auto trcnrrg = hss_.trcNrRange();
    trcnrrg.start = trcnrrg.stop = tnr;
    hss_.setTrcNrRange( trcnrrg );
}


LineSubSel::LineSubSel( const Bin2D& b2d )
    : LineSubSel(b2d.geomID(),b2d.trcNr())
{
}


LineSubSel::LineSubSel( const TrcKeySampling& tks )
    : LineSubSel( SurvGeom::get2D(tks.getGeomID()) )
{
    setTrcNrRange( tks.trcRange() );
}


LineSubSel::LineSubSel( const TrcKeyZSampling& tkzs )
    : LineSubSel( SurvGeom::get2D(tkzs.getGeomID()) )
{
    setTrcNrRange( tkzs.hsamp_.trcRange() );
}


const SurvGeom2D& LineSubSel::geometry2D() const
{
    return Geometry2D::get( geomID() );
}


void LineSubSel::merge( const LineSubSel& oth )
{
    hss_.merge( oth.hss_ );
    zss_.merge( oth.zss_ );
}


void LineSubSel::limitTo( const LineSubSel& oth )
{
    hss_.limitTo( oth.hss_ );
    zss_.limitTo( oth.zss_ );
}


LineSubSelSet::LineSubSelSet( const LineHorSubSelSet& lhsss )
{
    for ( auto lhss : lhsss )
	add( new LineSubSel( *lhss ) );
}


void LineSubSelSet::setToAll()
{
    setEmpty();
    GeomIDSet allgids;
    SurvGeom2D::getGeomIDs( allgids );
    for ( auto gid : allgids )
	add( new LineSubSel(gid) );
}


LineSubSelSet::totalsz_type LineSubSelSet::totalSize() const
{
    totalsz_type ret = 0;
    for ( auto lss : *this )
	ret += lss->totalSize();
    return ret;
}


bool LineSubSelSet::hasAllLines() const
{
    GeomIDSet allgids;
    SurvGeom2D::getGeomIDs( allgids );
    for ( auto gid : allgids )
	if ( !find(gid) )
	    return false;
    return true;
}


bool LineSubSelSet::isAll() const
{
    return hasAllLines() && hasFullRange() && hasFullZRange();
}


bool LineSubSelSet::hasFullRange() const
{
    for ( auto lss : *this )
	if ( !lss->hasFullRange() )
	    return false;
    return true;
}


bool LineSubSelSet::hasFullZRange() const
{
    for ( auto lss : *this )
	if ( !lss->zSubSel().hasFullRange() )
	    return false;
    return true;
}


void LineSubSelSet::merge( const LineSubSelSet& oth )
{
    for ( auto othlss : oth )
    {
	auto* lss = doFind( othlss->geomID() );
	if ( lss )
	    lss->merge( *othlss );
	else
	    add( new LineSubSel(*othlss) );
    }
}


void LineSubSelSet::limitTo( const LineSubSelSet& oth )
{
    ObjectSet<LineSubSel> torem;

    for ( auto lss : *this )
    {
	auto* othlss = oth.doFind( lss->geomID() );
	if ( !othlss )
	    torem.add( lss );
	else
	{
	    lss->trcNrSubSel().limitTo( othlss->trcNrSubSel() );
	    lss->zSubSel().limitTo( othlss->zSubSel() );
	}
    }

    for ( auto lss : torem )
	removeSingle( indexOf(lss) );
}


void LineSubSelSet::addStepout( trcnr_type so )
{
    for ( auto lss : *this )
	lss->trcNrSubSel().addStepout( so );
}


LineSubSel* LineSubSelSet::doFind( GeomID gid ) const
{
    for ( auto lss : *this )
	if ( lss->geomID() == gid )
	    return const_cast<LineSubSel*>( lss );
    return 0;
}


CubeSubSel::CubeSubSel( OD::SurvLimitType slt )
    : CubeSubSel( Geometry::get3D(slt) )
{
}


CubeSubSel::CubeSubSel( const Geometry3D& geom )
    : Survey::GeomSubSel( geom.zRange() )
    , hss_( geom )
{
}


CubeSubSel::CubeSubSel( const CubeHorSubSel& hss )
    : CubeSubSel( hss, Geometry::get3D().zRange() )
{
}


CubeSubSel::CubeSubSel( const CubeHorSubSel& hss, const z_steprg_type& zrg )
    : Survey::GeomSubSel( zrg )
    , hss_( hss )
{
}


CubeSubSel::CubeSubSel( const CubeHorSubSel& hss, const ZSubSel& zss )
    : CubeSubSel( hss, zss.outputZRange() )
{
}


CubeSubSel::CubeSubSel( const pos_steprg_type& inlrg,
			const pos_steprg_type& crlrg )
    : CubeSubSel( inlrg, crlrg, Geometry::get3D().zRange() )
{
}


CubeSubSel::CubeSubSel( const pos_steprg_type& inlrg,
			const pos_steprg_type& crlrg,
			const z_steprg_type& zrg )
    : Survey::GeomSubSel( zrg )
    , hss_( inlrg, crlrg )
{
}


CubeSubSel::CubeSubSel( const BinID& bid )
    : Survey::GeomSubSel( Geometry::get3D().zRange() )
    , hss_( bid )
{
}


CubeSubSel::CubeSubSel( const HorSampling& hsamp )
    : CubeSubSel( hsamp, Geometry::get3D().zRange() )
{
}


CubeSubSel::CubeSubSel( const HorSampling& hsamp, const z_steprg_type& zrg )
    : Survey::GeomSubSel( zrg )
    , hss_( hsamp )
{
}


CubeSubSel::CubeSubSel( const CubeSampling& cs )
    : CubeSubSel( cs.hsamp_, cs.zsamp_ )
{
}


CubeSubSel::CubeSubSel( const TrcKeySampling& tks )
    : Survey::GeomSubSel( SurvGeom::get3D().zRange() )
{
    setInlRange( tks.lineRange() );
    setCrlRange( tks.trcRange() );
}


CubeSubSel::CubeSubSel( const TrcKeyZSampling& tkzs )
    : Survey::GeomSubSel( SurvGeom::get3D().zRange() )
{
    setInlRange( tkzs.hsamp_.lineRange() );
    setCrlRange( tkzs.hsamp_.trcRange() );
    setZRange( tkzs.zsamp_ );
}


bool CubeSubSel::includes( const BinID& bid ) const
{
    return hss_.includes( bid );
}


void CubeSubSel::setRange( const BinID& start, const BinID& stop,
			   const BinID& stp )
{
    setInlRange( pos_steprg_type(start.inl(),stop.inl(),stp.inl()) );
    setCrlRange( pos_steprg_type(start.crl(),stop.crl(),stp.crl()) );
}


void CubeSubSel::merge( const CubeSubSel& oth )
{
    hss_.merge( oth.hss_ );
    zss_.merge( oth.zss_ );
}


void CubeSubSel::limitTo( const CubeSubSel& oth )
{
    hss_.limitTo( oth.hss_ );
    zss_.limitTo( oth.zss_ );
}


void CubeSubSel::setToAll()
{
    *this = CubeSubSel();
}


bool CubeSubSel::isFlat() const
{
    return nrInl() == 1 || nrCrl() == 1 || nrZ() == 1;
}


CubeSubSel::SliceType CubeSubSel::defaultDir() const
{
    const auto nrinl = nrInl();
    const auto nrcrl = nrCrl();
    const auto nrz = nrZ();
    return nrz < nrinl && nrz < nrcrl	? OD::ZSlice
		    : (nrinl<=nrcrl	? OD::InlineSlice
					: OD::CrosslineSlice);
}


CubeSubSel::size_type CubeSubSel::size( SliceType st ) const
{
    return st == OD::InlineSlice ?	nrInl()
	: (st == OD::CrosslineSlice ?	nrCrl()
				    :	nrZ());
}


void CubeSubSel::getDefaultNormal( Coord3& ret ) const
{
    const auto defdir = defaultDir();
    if ( defdir == OD::InlineSlice )
	ret = Coord3( SurvGeom::get3D().binID2Coord().inlDir(), 0 );
    else if ( defdir == OD::CrosslineSlice )
	ret = Coord3( SurvGeom::get3D().binID2Coord().crlDir(), 0 );
    else
	ret = Coord3( 0, 0, 1 );
}


mUseType( Survey::FullSubSel, z_type );
mUseType( Survey::FullSubSel, z_rg_type );
mUseType( Survey::FullSubSel, z_steprg_type );
mUseType( Survey::FullSubSel, pos_type );
mUseType( Survey::FullSubSel, pos_rg_type );
mUseType( Survey::FullSubSel, idx_type );
mUseType( Survey::FullSubSel, size_type );
typedef StepInterval<pos_type> pos_steprg_type;
typedef StepInterval<z_type> z_steprg_type;

const char* Survey::FullSubSel::sNrLinesKey()
{
    return IOPar::compKey( sKey::Line(mPlural),sKey::Size() );
}


Survey::FullSubSel::FullSubSel()
    : css_(new CubeSubSel)
    , lsss_(*new LineSubSelSet)
{
}


Survey::FullSubSel::FullSubSel( GeomID gid )
    : lsss_(*new LineSubSelSet)
{
    if ( gid.is2D() )
	lsss_ += new LineSubSel( gid );
    else
	css_ = new CubeSubSel;
}


Survey::FullSubSel::FullSubSel( const BinID& bid )
    : FullSubSel( TrcKey(bid) )
{
}


Survey::FullSubSel::FullSubSel( GeomID gid, trcnr_type tnr )
    : FullSubSel( TrcKey(gid,tnr) )
{
}


Survey::FullSubSel::FullSubSel( const TrcKey& tk )
    : lsss_(*new LineSubSelSet)
{
    if ( tk.is2D() )
	lsss_ += new LineSubSel( tk.geomID(), tk.trcNr() );
    else
	css_ = new CubeSubSel( tk.binID() );
}


Survey::FullSubSel::FullSubSel( const GeomIDSet& gids )
    : lsss_(*new LineSubSelSet)
{
    if ( gids.isEmpty() || !gids.first().is2D() )
	css_ = new CubeSubSel;
    else
	for ( auto gid : gids )
	    lsss_ += new LineSubSel( gid );
}


Survey::FullSubSel::FullSubSel( const CubeSubSel& css )
    : css_(new CubeSubSel(css))
    , lsss_(*new LineSubSelSet)
{
}


Survey::FullSubSel::FullSubSel( const LineSubSel& lss )
    : lsss_(*new LineSubSelSet)
{
    lsss_ += new LineSubSel( lss );
}


Survey::FullSubSel::FullSubSel( const GeomSubSel& gss )
    : lsss_(*new LineSubSelSet)
{
    if ( gss.is2D() )
	lsss_ += new LineSubSel( *gss.asLineSubSel() );
    else
	css_ = new CubeSubSel( *gss.asCubeSubSel() );
}


Survey::FullSubSel::FullSubSel( const CubeHorSubSel& chss )
    : css_(new CubeSubSel(chss))
    , lsss_(*new LineSubSelSet)
{
}


Survey::FullSubSel::FullSubSel( const LineHorSubSel& lhss )
    : lsss_(*new LineSubSelSet)
{
    lsss_ += new LineSubSel( lhss );
}


Survey::FullSubSel::FullSubSel( const LineSubSelSet& lsss )
    : lsss_(*new LineSubSelSet)
{
    deepCopy( lsss_, lsss );
}


Survey::FullSubSel::FullSubSel( const LineHorSubSelSet& lhsss )
    : lsss_(*new LineSubSelSet)
{
    for ( auto lhss : lhsss )
	lsss_ += new LineSubSel( *lhss );
}


Survey::FullSubSel::FullSubSel( const TrcKeySampling& tks )
    : lsss_(*new LineSubSelSet)
{
    if ( tks.is2D() )
	lsss_ += new LineSubSel( tks );
    else
	css_ = new CubeSubSel( tks );
}


Survey::FullSubSel::FullSubSel( const TrcKeyZSampling& tkzs )
    : lsss_(*new LineSubSelSet)
{
    if ( tkzs.is2D() )
	lsss_ += new LineSubSel( tkzs );
    else
	css_ = new CubeSubSel( tkzs );
}


Survey::FullSubSel::FullSubSel( const FullSubSel& oth )
    : lsss_(*new LineSubSelSet)
{
    *this = oth;
}


Survey::FullSubSel::FullSubSel( const IOPar& iop )
    : FullSubSel()
{
    usePar( iop );
}


Survey::FullSubSel::~FullSubSel()
{
    clearContents();
    delete &lsss_;
}


Survey::FullSubSel& Survey::FullSubSel::operator=( const FullSubSel& oth )
{
    if ( this != &oth )
    {
	clearContents();
	if ( oth.css_ )
	    css_ = new CubeSubSel( *oth.css_ );
	else
	    lsss_ = oth.lsss_;
    }
    return *this;
}


void Survey::FullSubSel::clearContents()
{
    deleteAndZeroPtr( css_ );
    lsss_.setEmpty();
}


void Survey::FullSubSel::set3D( bool yn )
{
    if ( !yn )
	deleteAndZeroPtr( css_ );
    else
    {
	lsss_.setEmpty();
	if ( !css_ )
	    css_ = new CubeSubSel;
    }
}


void Survey::FullSubSel::merge( const FullSubSel& oth )
{
    if ( oth.is2D() != is2D() )
	{ pErrMsg("2D/3D mismatch"); return; }
    if ( oth.css_ )
	css_->merge( *oth.css_ );
    else
	lsss_.merge( oth.lsss_ );
}


void Survey::FullSubSel::limitTo( const FullSubSel& oth )
{
    if ( oth.is2D() != is2D() )
	{ pErrMsg("2D/3D mismatch"); return; }
    if ( oth.css_ )
	css_->limitTo( *oth.css_ );
    else
	lsss_.limitTo( oth.lsss_ );
}


bool Survey::FullSubSel::isAll() const
{
    if ( css_ )
	return css_->isAll();
    else
	return lsss_.isAll();
}


bool Survey::FullSubSel::isFlat() const
{
    if ( css_ )
	return css_->isFlat();
    else
	return true;
}


bool Survey::FullSubSel::isZSlice() const
{
    if ( css_ )
	return isFlat() && css_->defaultDir() == OD::ZSlice;
    else
	return !lsss_.isEmpty() && lsss_.first()->zRange().nrSteps() < 2;
}


static pos_type getInlCrl42D( const SurvGeom2D& geom,
			      bool first, bool inl )
{
    const Coord coord( geom.getCoordByIdx(first? 0 : geom.size()-1) );
    const BinID bid( SI().transform(coord) );
    return inl ? bid.inl() : bid.crl();
}


static void inclInlCrl42D( const SurvGeom2D& geom,
			   pos_rg_type& rg, bool inl )
{
    if ( geom.isEmpty() )
	return;

    auto posidx = getInlCrl42D( geom, true, inl );
    if ( mIsUdf(rg.start) )
	rg.start = rg.stop = posidx;
    else
	rg.include( posidx, false );
    posidx = getInlCrl42D( geom, false, inl );
    rg.include( posidx );
}


pos_steprg_type Survey::FullSubSel::inlRange() const
{
    if ( css_ )
	return css_->inlRange();

    pos_rg_type ret( mUdf(int), mUdf(int) );
    for ( auto idx=0; idx<nrGeomIDs(); idx++ )
	inclInlCrl42D( SurvGeom2D::get( geomID(idx) ), ret, true );

    return pos_steprg_type( ret, SI().inlStep() );
}


pos_steprg_type Survey::FullSubSel::crlRange() const
{
    if ( css_ )
	return css_->crlRange();

    pos_rg_type ret( mUdf(int), mUdf(int) );
    for ( auto idx=0; idx<nrGeomIDs(); idx++ )
	inclInlCrl42D( SurvGeom2D::get( geomID(idx) ), ret, false );

    return pos_steprg_type( ret, SI().crlStep() );
}



pos_steprg_type Survey::FullSubSel::trcNrRange( idx_type iln ) const
{
    if ( css_ )
	return crlRange();
    else if ( iln >= lsss_.size() )
	{ pErrMsg("iln>=sz"); return pos_rg_type(); }

    return lsss_.get(iln)->trcNrRange();
}


z_steprg_type Survey::FullSubSel::zRange( idx_type iln ) const
{
    if ( css_ )
	return css_->zRange();
    if ( !lsss_.validIdx(iln) )
	return SI().zRange();
    return lsss_.get(iln)->zRange();
}


size_type Survey::FullSubSel::nrGeomIDs() const
{
    return css_ ? 1 : lsss_.size();
}


Pos::GeomID Survey::FullSubSel::geomID( idx_type iln ) const
{
    if ( css_ )
	return GeomID::get3D();
    else if ( iln >= lsss_.size() )
	return GeomID();
    return lsss_.get(iln)->geomID();
}


idx_type Survey::FullSubSel::indexOf( GeomID gid ) const
{
    if ( !is2D() )
	return gid.is2D() ? -1 : 0;
    for ( int idx=0; idx<lsss_.size(); idx++ )
	if ( lsss_.get(idx)->geomID() == gid )
	    return idx;
    return -1;
}


void Survey::FullSubSel::setInlRange( const pos_rg_type& rg )
{
    if ( !css_ )
	return;
    if ( rg.hasStep() )
	css_->setInlRange( (const pos_steprg_type&)rg );
    else
	css_->setInlRange( pos_steprg_type(rg,SI().inlStep()) );
}


void Survey::FullSubSel::setCrlRange( const pos_rg_type& rg )
{
    if ( !css_ )
	return;
    if ( rg.hasStep() )
	css_->setCrlRange( (const pos_steprg_type&)rg );
    else
	css_->setCrlRange( pos_steprg_type(rg,SI().crlStep()) );
}


void Survey::FullSubSel::setTrcNrRange( GeomID gid, const pos_rg_type& rg )
{
    const auto idxof = indexOf( gid );
    if ( idxof >= 0 )
	setTrcNrRange( rg, idxof );
}


void Survey::FullSubSel::setTrcNrRange( const pos_rg_type& rg, idx_type idx )
{
    if ( !is2D() )
	setCrlRange( rg );
    else if ( idx >= lsss_.size() )
	{ pErrMsg("idx bad"); }
    else
    {
	auto& lss = *lsss_[idx];
	if ( rg.hasStep() )
	    lss.setTrcNrRange( (const pos_steprg_type&)rg );
	else
	    lss.setTrcNrRange( pos_steprg_type(rg,lss.zRange().step) );
    }
}


void Survey::FullSubSel::setZRange( const z_rg_type& rg, idx_type idx )
{
    Survey::GeomSubSel* gss = 0;
    if ( css_ )
	gss = css_;
    else if ( idx < lsss_.size() )
	gss = lsss_[idx];
    if ( !gss )
	return;

    if ( rg.hasStep() )
	gss->setZRange( (const z_steprg_type&)rg );
    else
	gss->setZRange( z_steprg_type(rg,gss->zRange().step) );
}


void Survey::FullSubSel::setToNone( bool is2d )
{
    clearContents();
    set3D( !is2d );
}


void Survey::FullSubSel::setToAll( bool is2d )
{
    clearContents();
    if ( is2d )
	lsss_.setToAll();
    else
	css_ = new CubeSubSel;
}


void Survey::FullSubSel::set( const CubeSubSel& css )
{
    set3D( true );
    *css_ = css;
}


void Survey::FullSubSel::set( const LineSubSel& lss )
{
    set3D( false );
    lsss_.setEmpty();
    lsss_.add( new LineSubSel(lss) );
}


void Survey::FullSubSel::set( const LineSubSelSet& lsss )
{
    set3D( false );
    lsss_ = lsss;
}


void Survey::FullSubSel::setGeomID( GeomID gid )
{
    if ( is2D() )
    {
	if ( lsss_.size() == 1 && lsss_.get(0)->geomID() == gid )
	    return;
	lsss_.setEmpty();
    }

    addGeomID( gid );
}


void Survey::FullSubSel::addGeomID( GeomID gid )
{
    set3D( !gid.is2D() );
    if ( is2D() )
    {
	const auto idxof = indexOf( gid );
	if ( idxof < 0 )
	    lsss_.add( new LineSubSel( gid ) );
    }
}


void Survey::FullSubSel::fillPar( IOPar& iop ) const
{
    if ( css_ )
	css_->fillPar( iop );
    else
    {
	iop.set( sNrLinesKey(), lsss_.size() );
	for ( int idx=0; idx<lsss_.size(); idx++ )
	{
	    IOPar subiop;
	    lsss_.get(idx)->fillPar( subiop );
	    iop.mergeComp( subiop, toString(idx) );
	}
    }
}


void Survey::FullSubSel::usePar( const IOPar& iop )
{
    clearContents();
    set3D( !iop.isPresent(sNrLinesKey()) );

    if ( css_ )
	css_->usePar( iop );
    else
    {
	size_type nrlines = 0;
	iop.get( sNrLinesKey(), nrlines );
	for ( auto idx=0; idx<nrlines; idx++ )
	{
	    PtrMan<IOPar> subpar = iop.subselect( toString(idx) );
	    if ( subpar && !subpar->isEmpty() )
	    {
		bool is2d = true; GeomID gid;
		Survey::SubSel::getInfo( *subpar, is2d, gid );
		if ( is2d && gid.isValid() )
		{
		    auto* lss = new LineSubSel( gid );
		    lss->usePar( iop );
		    lsss_ += lss;
		}
	    }
	}
    }
}


int Survey::FullSubSel::selRes3D( const BinID& bid ) const
{
    int ret;
    if ( css_ )
    {
	const auto inlrg = css_->inlRange();
	const auto crlrg = css_->crlRange();
	int inlres = inlrg.start > bid.inl() || inlrg.stop < bid.inl() ? 2 : 0;
	int crlres = crlrg.start > bid.crl() || crlrg.stop < bid.crl() ? 2 : 0;
	ret = inlres + 256 * crlres;
	if ( ret == 0 )
	{
	    inlres = (bid.inl() - inlrg.start) % inlrg.step ? 1 : 0;
	    crlres = (bid.crl() - crlrg.start) % crlrg.step ? 1 : 0;
	    ret = inlres + 256 * crlres;
	}
    }
    else
    {
	const Coord coord = SI().transform( bid );
	ret = 2 + 256 * 2;
	for ( int iln=0; iln<lsss_.size(); iln++ )
	{
	    const auto& lss = *lsss_[iln];
	    const auto rg = lss.trcNrRange();
	    const auto& geom = lss.geometry2D();
	    auto trcnr = geom.nearestTracePosition( coord );
	    if ( trcnr >= 0 )
	    {
		const BinID trcbid( SI().transform(geom.getCoord(trcnr)) );
		if ( trcbid == bid )
		    { ret = 0; break; }
	    }
	}
    }

    return ret;
}


int Survey::FullSubSel::selRes2D( GeomID gid, pos_type trcnr ) const
{
    int ret;
    const auto lidx = indexOf( gid );
    if ( lidx < 0 )
	ret = 2 + 256 * 2;
    else
    {
	const auto rg = lsss_[lidx]->trcNrRange();
	int res = rg.start > trcnr || rg.stop < trcnr ? 2 : 0;
	if ( res == 0 )
	    res = (trcnr - rg.start) % rg.step ? 1 : 0;
	ret = 256 * res;
    }
    return ret;
}


uiString Survey::FullSubSel::getUserSummary() const
{
    if ( !css_ )
    {
	if ( lsss_.size() < 1 )
	    return toUiString( "-" );

	const auto nrtrcs = expectedNrTraces();
	const auto nrlines = lsss_.size();
	if ( nrlines > 1 )
	    return toUiString("%1: %2 (%3: %4)")
			.arg( uiStrings::sLine(nrlines) ).arg( nrlines )
			.arg( uiStrings::sTrace(nrtrcs) ).arg( nrtrcs );
	const auto& lss = *lsss_.first();
	const auto nrrg = lss.trcNrRange();
	return toUiString( "%1-%2 [%3]" )
		    .arg( nrrg.start ).arg( nrrg.stop )
		    .arg( lss.geomID().name() );
    }

    const auto& inlss = css_->inlSubSel(); const auto inlsz = inlss.size();
    const auto& crlss = css_->crlSubSel(); const auto crlsz = crlss.size();
    if ( inlsz < 1 || crlsz < 1 )
	return uiStrings::sRemove();
    if ( inlsz < 2 && crlsz < 2 )
	return toUiString( "%1/%2" ).arg( inlss.posStart() )
				    .arg( crlss.posStart() );
    else if ( inlsz < 2 )
	return toUiString( "%1: %2" ).arg( uiStrings::sInline() )
				     .arg( inlss.posStart() );
    else if ( crlsz < 2 )
	return toUiString( "%1: %2" ).arg( uiStrings::sCrossline() )
				     .arg( crlss.posStart() );

    return toUiString( "%2/%3-%4/%5" )
			 .arg( inlss.posStart() ).arg( crlss.posStart() )
			 .arg( inlss.posStop() ).arg( crlss.posStop() );
}


size_type Survey::FullSubSel::expectedNrTraces() const
{
    size_type ret = 0;
    if ( css_ )
	ret = css_->nrInl() * css_->nrCrl();
    else
	for ( auto lss : lsss_ )
	    ret += lss->nrTrcs();
    return ret;
}


Survey::HorSubSel& Survey::FullSubSel::horSubSel( idx_type idx )
{
    return geomSubSel( idx ).horSubSel();
}


const Survey::HorSubSel& Survey::FullSubSel::horSubSel( idx_type idx ) const
{
    return geomSubSel( idx ).horSubSel();
}


Pos::ZSubSel& Survey::FullSubSel::zSubSel( idx_type idx )
{
    return geomSubSel( idx ).zSubSel();
}


const Pos::ZSubSel& Survey::FullSubSel::zSubSel( idx_type idx ) const
{
    return geomSubSel( idx ).zSubSel();
}


const Survey::GeomSubSel& Survey::FullSubSel::geomSubSel( idx_type idx ) const
{
    return mSelf().geomSubSel( idx );
}


Survey::GeomSubSel& Survey::FullSubSel::geomSubSel( idx_type idx )
{
    if ( css_ )
	return *css_;
    else if ( lsss_.validIdx(idx) )
	return *lsss_.get( idx );
    else
    {
	pErrMsg("Bad idx");
	static LineSubSel dum = LineSubSel( GeomID() );
	return dum;
    }
}


LineSubSel& Survey::FullSubSel::lineSubSel( idx_type idx )
{
    return *lsss_.get( idx );
}


const LineSubSel& Survey::FullSubSel::lineSubSel( idx_type idx ) const
{
    return *lsss_.get( idx );
}


bool Survey::FullSubSel::hasFullZRange() const
{
    return css_ ? css_->zSubSel().isAll() : lsss_.hasFullZRange();
}


const LineSubSel* Survey::FullSubSel::findLineSubSel( GeomID geomid ) const
{
    return lsss_.find( geomid );
}


Survey::FullSubSelPosIter::FullSubSelPosIter( const FullSubSel& fss )
    : subsel_(fss)
{
}


Survey::FullSubSelPosIter::FullSubSelPosIter( const FullSubSelPosIter& oth )
    : subsel_(oth.subsel_)
    , lineidx_(oth.lineidx_)
    , trcidx_(oth.trcidx_)
{
}


#define mRetNoNext() { reset(); return false; }

bool Survey::FullSubSelPosIter::next()
{
    if ( lineidx_ < 0 )
	lineidx_ = 0;
    trcidx_++;

    if ( subsel_.css_ )
    {
	if ( trcidx_ >= subsel_.css_->crlSubSel().size() )
	{
	    lineidx_++; trcidx_ = 0;
	    if ( lineidx_ >= subsel_.css_->inlSubSel().size() )
		mRetNoNext()
	}
    }
    else
    {
	if ( lineidx_ >= subsel_.lsss_.size() )
	    mRetNoNext()
	const auto& lss = *subsel_.lsss_.get( lineidx_ );
	if ( trcidx_ >= lss.trcNrSubSel().size() )
	{
	    lineidx_++; trcidx_ = 0;
	    if ( lineidx_ >= subsel_.lsss_.size() )
		mRetNoNext()
	}
    }

    return true;
}


#define mLineIdx() (lineidx_<0 ? 0 : lineidx_)
#define mTrcIdx() (trcidx_<0 ? 0 : trcidx_)


Pos::GeomID Survey::FullSubSelPosIter::geomID() const
{
    if ( !subsel_.is2D() )
	return GeomID::get3D();

    const auto lidx = mLineIdx();
    return lidx < subsel_.lsss_.size() ? subsel_.lsss_.get(lidx)->geomID()
				       : GeomID();
}


pos_type Survey::FullSubSelPosIter::trcNr() const
{
    const auto tidx = mTrcIdx();
    if ( !subsel_.is2D() )
	return subsel_.css_->crlSubSel().pos4Idx( tidx );

    const auto lidx = mLineIdx();
    if ( lidx >= subsel_.lsss_.size() )
	return 0;

    return subsel_.lsss_.get(lidx)->trcNrSubSel().pos4Idx( tidx );
}


BinID Survey::FullSubSelPosIter::binID() const
{
    if ( !subsel_.is2D() )
	return subsel_.css_->binID4RowCol( RowCol(mTrcIdx(),mLineIdx()) );

    const auto lidx = mLineIdx();
    if ( lidx >= subsel_.lsss_.size() )
	return BinID::udf();

    const auto& lss = *subsel_.lsss_.get( lidx );
    const auto trcnr = lss.trcNrSubSel().pos4Idx( mTrcIdx() );
    const auto coord = lss.geometry2D().getCoord( trcnr );
    return SI().transform( coord );
}
