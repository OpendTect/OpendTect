/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 2019
-*/


#include "survsubsel.h"
#include "cubesampling.h"
#include "cubesubsel.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "survgeom2d.h"
#include "survgeom3d.h"
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


LineHorSubSel::LineHorSubSel( const LineSubSel& lss )
    : LineHorSubSel( lss.lineHorSubSel() )
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


const SurvGeom2D& LineHorSubSel::geometry2D() const
{
    return Geometry2D::get( geomID() );
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


void LineHorSubSelSet::setToAll()
{
    setEmpty();
    GeomIDSet allgids;
    SurvGeom2D::getGeomIDs( allgids );
    for ( auto gid : allgids )
	add( new LineHorSubSel(gid) );
}


Bin2D LineHorSubSelSet::atGlobIdx( totalsz_type gidx ) const
{
    totalsz_type totsz = 0;
    for ( auto lhss : *this )
    {
	auto cursz = lhss->totalSize();
	if ( totsz+cursz > gidx )
	{
	    const idx_type locidx = (idx_type)(gidx - totsz);
	    return Bin2D( lhss->geomID(), lhss->trcNr4Idx(locidx) );
	}

	totsz += cursz;
    }

    return Bin2D::udf();
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


CubeHorSubSel::CubeHorSubSel( const CubeSubSel& css )
    : CubeHorSubSel( css.cubeHorSubSel() )
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
