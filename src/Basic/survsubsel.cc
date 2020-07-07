/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 2019
-*/


#include "survsubsel.h"
#include "cubesampling.h"
#include "cubesubsel.h"
#include "fullsubsel.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "odjson.h"
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
	if ( iop.get(sKey::SurveyID(),igs) )
	{
	    if ( igs < (int)OD::SynthGeom || igs > (int)OD::LineBasedGeom )
		igs = (int)OD::VolBasedGeom;
	}
	else
	    igs = (int)OD::LineBasedGeom;
    }

    is2d = igs == (int)OD::LineBasedGeom;
    if ( is2d )
	return iop.get( sKey::GeomID(), geomid )
	    && SurvGeom::isUsable( geomid );

    return true;
}


bool Survey::SubSel::getInfo( const OD::JSON::Object& obj, bool& is2d,
			      GeomID& geomid )
{
    is2d = false;
    geomid = GeomID::get3D();

    int igs = (int)OD::VolBasedGeom;
    if ( obj.isPresent(sKey::GeomSystem()) )
    {
	igs = mCast(int,obj.getIntValue(sKey::GeomSystem()));
    }
    else
    {
	if ( obj.isPresent(sKey::SurveyID()) )
	{
	    igs = mCast(int,obj.getIntValue(sKey::SurveyID()));
	    if ( igs < (int)OD::SynthGeom || igs > (int)OD::LineBasedGeom )
		igs = (int)OD::VolBasedGeom;
	}
	else
	    igs = (int)OD::LineBasedGeom;
    }

    is2d = igs == (int)OD::LineBasedGeom;
    if ( is2d )
	return obj.getGeomID( sKey::GeomID(), geomid )
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


void Survey::SubSel::fillJSONInfo( OD::JSON::Object& obj, bool is2d,
				   GeomID gid )
{
    const OD::GeomSystem gs = is2d ? OD::LineBasedGeom : OD::VolBasedGeom;
    obj.set( sKey::GeomSystem(), (int)gs );
    if ( is2d )
	obj.set( sKey::GeomID(), gid );
    else
	obj.remove( sKey::GeomID() );
}


const SurveyInfo& Survey::HorSubSel::survInfo() const
{
    return SI( si_ );
}


Survey::HorSubSel* Survey::HorSubSel::duplicate() const
{
    if ( is2D() )
	return new LineHorSubSel( *asLineHorSubSel() );
    else
	return new CubeHorSubSel( *asCubeHorSubSel() );
}


LineHorSubSel* Survey::HorSubSel::asLineHorSubSel()
{
    return is2D() ? static_cast<LineHorSubSel*>( this ) : nullptr;
}


const LineHorSubSel* Survey::HorSubSel::asLineHorSubSel() const
{
    return is2D() ? static_cast<const LineHorSubSel*>( this ) : nullptr;
}


CubeHorSubSel* Survey::HorSubSel::asCubeHorSubSel()
{
    return is2D() ? nullptr : static_cast<CubeHorSubSel*>( this );
}


const CubeHorSubSel* Survey::HorSubSel::asCubeHorSubSel() const
{
    return is2D() ? nullptr : static_cast<const CubeHorSubSel*>( this );
}


Survey::HorSubSel* Survey::HorSubSel::get( const TrcKeySampling& tks )
{
    if ( tks.is2D() )
	return new LineHorSubSel( tks );
    else
	return new CubeHorSubSel( tks );
}


bool Survey::HorSubSel::includes( const BinID& bid ) const
{
    const auto* chss = asCubeHorSubSel();
    return chss ? chss->includes( bid ) : false;
}


bool Survey::HorSubSel::includes( const Bin2D& b2d ) const
{
    const auto* lhss = asLineHorSubSel();
    return lhss ? lhss->includes( b2d ) : false;
}


bool Survey::HorSubSel::includes( const TrcKey& tk ) const
{
    return tk.is2D() ? includes( tk.bin2D() ) : includes( tk.binID() );
}


Pos::IdxPair Survey::HorSubSel::atGlobIdx( totalsz_type gidx ) const
{
    const LineHorSubSel* lhss = asLineHorSubSel();
    const CubeHorSubSel* chss = asCubeHorSubSel();
    if ( lhss )
	return lhss->atGlobIdx(gidx).idxPair();
    if ( chss )
	return chss->atGlobIdx(gidx);
    return Pos::IdxPair::udf();
}


Survey::HorSubSel* Survey::HorSubSel::create( const IOPar& iop,
					      const SurveyInfo* si )
{
    bool is2d; GeomID gid;
    if ( !getInfo(iop,is2d,gid) )
	return 0;

    HorSubSel* ret = 0;
    if ( is2d )
	ret = new LineHorSubSel( gid );
    else
	ret = new CubeHorSubSel( si );
    if ( ret )
	ret->usePar( iop );

    return ret;
}


bool Survey::HorSubSel::usePar( const IOPar& iop )
{
    return doUsePar( iop );
}


bool Survey::HorSubSel::useJSON( const OD::JSON::Object& obj )
{
    return doUseJSON( obj );
}


void Survey::HorSubSel::fillPar( IOPar& iop ) const
{
    fillParInfo( iop, is2D(), geomID() );
    doFillPar( iop );
}


void Survey::HorSubSel::fillJSON( OD::JSON::Object& obj ) const
{
    fillJSONInfo( obj, is2D(), geomID() );
    doFillJSON( obj );
}


Survey::HorSubSel::pos_type Survey::HorSubSel::lineNr4Idx( idx_type idx ) const
{
    return is2D() ? 0 : asCubeHorSubSel()->inl4Idx( idx );
}


Survey::HorSubSel::pos_type Survey::HorSubSel::trcNr4Idx( idx_type idx ) const
{
    return is2D() ? asLineHorSubSel()->trcNr4Idx( idx )
		  : asCubeHorSubSel()->crl4Idx( idx );
}


Survey::HorSubSel::idx_type Survey::HorSubSel::idx4LineNr( pos_type idx ) const
{
    return is2D() ? 0 : asCubeHorSubSel()->idx4Inl( idx );
}


Survey::HorSubSel::idx_type Survey::HorSubSel::idx4TrcNr( pos_type idx ) const
{
    return is2D() ? asLineHorSubSel()->idx4TrcNr( idx )
		  : asCubeHorSubSel()->idx4Crl( idx );
}


Survey::HorSubSelIterator::HorSubSelIterator( const HorSubSel& hss )
    : hss_(const_cast<HorSubSel&>(hss))
    , nrtrcs_(hss.trcNrRange().nrSteps()+1)
{
    toStart();
}


Survey::HorSubSelIterator::HorSubSelIterator( HorSubSel& hss )
    : hss_(hss)
    , nrtrcs_(hss.trcNrRange().nrSteps()+1)
{
    toStart();
}


bool Survey::HorSubSelIterator::next()
{
    tidx_++;
    if ( tidx_ >= nrtrcs_ )
    {
	tidx_ = -1;
	if ( is2D() )
	    return false;
	lidx_++;
	if ( lidx_ >= hss_.asCubeHorSubSel()->nrInl() )
	    { lidx_ = 0; return false; }
	tidx_++;
    }
    return true;
}


BinID Survey::HorSubSelIterator::binID() const
{
    if ( !is2D() )
    {
	const auto& chss = *hss_.asCubeHorSubSel();
	return BinID( chss.inl4Idx(lidx_), chss.crl4Idx(tidx_) );
    }

    const auto b2d( bin2D() );
    return Survey::Geometry::get3D().transform( b2d.coord() );
}


Bin2D Survey::HorSubSelIterator::bin2D() const
{
    if ( is2D() )
    {
	const auto& lhss = *hss_.asLineHorSubSel();
	return Bin2D( lhss.geomID(), lhss.trcNr4Idx(tidx_) );
    }

    //TODO find nearest
    return Bin2D::udf();
}


void Survey::HorSubSelIterator::getTrcKey( TrcKey& tk ) const
{
    if ( is2D() )
	tk.setPos( bin2D() );
    else
	tk.setPos( binID() );
}


Survey::GeomSubSel::GeomSubSel( const z_steprg_type& zrg )
    : zss_( zrg )
{
}


bool Survey::GeomSubSel::includes( const GeomSubSel& oth ) const
{
    const bool is2d = is2D();
    if ( is2d != oth.is2D() )
	return false;

    return is2d ? asLineSubSel()->includes( *oth.asLineSubSel() )
		: asCubeSubSel()->includes( *oth.asCubeSubSel() );
}


Survey::GeomSubSel* Survey::GeomSubSel::duplicate() const
{
    if ( is2D() )
	return new LineSubSel( *asLineSubSel() );
    else
	return new CubeSubSel( *asCubeSubSel() );
}


LineSubSel* Survey::GeomSubSel::asLineSubSel()
{
    return is2D() ? static_cast<LineSubSel*>( this ) : nullptr;
}


const LineSubSel* Survey::GeomSubSel::asLineSubSel() const
{
    return is2D() ? static_cast<const LineSubSel*>( this ) : nullptr;
}


CubeSubSel* Survey::GeomSubSel::asCubeSubSel()
{
    return is2D() ? nullptr : static_cast<CubeSubSel*>( this );
}


const CubeSubSel* Survey::GeomSubSel::asCubeSubSel() const
{
    return is2D() ? nullptr : static_cast<const CubeSubSel*>( this );
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


bool Survey::GeomSubSel::useJSON( const OD::JSON::Object& obj )
{
    if ( !horSubSel().useJSON(obj) )
	return false;
    zss_.useJSON( obj );
    return true;
}


void Survey::GeomSubSel::fillPar( IOPar& iop ) const
{
    horSubSel().fillPar( iop );
    zss_.fillPar( iop );
}


void Survey::GeomSubSel::fillJSON( OD::JSON::Object& obj ) const
{
    horSubSel().fillJSON( obj );
    zss_.fillJSON( obj );
}


Survey::GeomSubSel::dist_type Survey::GeomSubSel::trcDist( bool max ) const
{
    if ( !is2D() )
    {
	const CubeSubSel& css = *asCubeSubSel();
	const auto& g3d = Geometry::get3D();
	const auto inldist = css.inlRange().step * g3d.inlDistance();
	const auto crldist = css.crlRange().step * g3d.crlDistance();
	return max ? (inldist > crldist ? inldist : crldist)
		   : (inldist + crldist) / 2;
    }

    const LineSubSel& lss = *asLineSubSel();
    const auto& g2d = lss.geometry2D();
    const auto trcnrrg = lss.trcNrRange();
    auto prevcoord( Coord::udf() );
    dist_type sqd = 0;
    auto nrdists = 0;
    for ( auto tnr=trcnrrg.start; tnr<=trcnrrg.stop; tnr += trcnrrg.step )
    {
	const auto curcoord( g2d.getCoord(tnr) );
	if ( prevcoord.isUdf() )
	    continue;

	const auto distsq = curcoord.sqDistTo( prevcoord );
	if ( max )
	    { if ( sqd < distsq ) sqd = distsq; }
	else
	    sqd += distsq;

	nrdists++;
	prevcoord = curcoord;
    }

    if ( nrdists < 1 )
	return g2d.averageTrcDist();

    return max ? sqd : sqd / nrdists;
}


void Survey::GeomSubSel::limitTo( const GeomSubSel& oth )
{
    const bool is2d = is2D();
    if ( is2d != oth.is2D() )
	{ pErrMsg("2D/3D err"); return; }
    if ( is2d )
	asLineSubSel()->limitTo( *oth.asLineSubSel() );
    else
	asCubeSubSel()->limitTo( *oth.asCubeSubSel() );
}


void Survey::GeomSubSel::merge( const GeomSubSel& oth )
{
    const bool is2d = is2D();
    if ( is2d != oth.is2D() )
	{ pErrMsg("2D/3D err"); return; }
    if ( is2d )
	asLineSubSel()->merge( *oth.asLineSubSel() );
    else
	asCubeSubSel()->merge( *oth.asCubeSubSel() );
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


bool LineHorSubSel::equals( const SubSel& ss ) const
{
    mDynamicCastGet( const LineHorSubSel*, oth, &ss );
    if ( !oth )
	return false;

    return geomid_ == oth->geomid_ &&
	   ssdata_.sameOutputPosRange( oth->ssdata_ );
}


const SurvGeom2D& LineHorSubSel::geometry2D() const
{
    return Geometry2D::get( geomID() );
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


bool LineHorSubSel::getIntersection( const LineHorSubSel& oth,
				     LineHorSubSel& out ) const
{
    if ( oth.geomID() != geomID() )
	return false;

    LineHorSubSel lhss1( *this ); lhss1.normalise();
    LineHorSubSel lhss2( oth );   lhss2.normalise();
    const pos_steprg_type trcrg1( lhss1.trcNrRange() );
    const pos_steprg_type trcrg2( lhss2.trcNrRange() );
    pos_steprg_type commontrcrg;
    const bool success = Pos::intersect( trcrg1, trcrg2, commontrcrg );
    if ( success )
    {
	out.setGeomID( geomID() );
	out.setTrcNrRange( commontrcrg );
    }
    return true;
}


void LineHorSubSel::normalise()
{
    Pos::steprg_type trcnrrg( trcNrRange() );
    const LineSubSel lss( geomID() );
    Pos::normalise( trcnrrg, lss.trcDist(false) );
    setTrcNrRange( trcnrrg );
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
    if ( !iop->get(sKey::GeomID(),geomid) || !SurvGeom::isPresent(geomid) )
	return false;

    auto trcrg( trcNrRange() );
    *this = LineHorSubSel( geomid ); // clear to no subselection
    if ( !iop->get(sKey::FirstTrc(),trcrg.start) ||
	 !iop->get(sKey::LastTrc(),trcrg.stop) )
	iop->get(sKey::TrcRange(),trcrg );
    else
    {
	if ( !iop->get(sKey::StepTrc(),trcrg.step) )
	    iop->get( sKey::StepCrl(), trcrg.step );
    }
    ssdata_.setOutputPosRange( trcrg );

    return true;
}


bool LineHorSubSel::doUseJSON( const OD::JSON::Object& inpobj )
{
    const OD::JSON::Object* obj = &inpobj;
    if ( !obj->isPresent(sKey::GeomID()) )
    {
	obj = inpobj.getObject( sKey::Line() );
	if ( !obj )
	    return false;
    }

    auto geomid = geomid_;
    if ( !obj->getGeomID(sKey::GeomID(),geomid) ||
	 !SurvGeom::isPresent(geomid) )
	return false;

    auto trcrg( trcNrRange() );
    *this = LineHorSubSel( geomid ); // clear to no subselection
    obj->get( sKey::TrcRange(), trcrg );
    ssdata_.setOutputPosRange( trcrg );

    return true;
}


void LineHorSubSel::doFillPar( IOPar& iop ) const
{
    const auto trcrg( trcNrRange() );
    iop.set( sKey::FirstTrc(), trcrg.start );
    iop.set( sKey::LastTrc(), trcrg.stop );
    iop.set( sKey::StepTrc(), trcrg.step );
}


void LineHorSubSel::doFillJSON( OD::JSON::Object& obj ) const
{
    obj.set( sKey::TrcRange(), trcNrRange() );
}


LineHorSubSelSet::LineHorSubSelSet( GeomID gid )
{
    add( new LineHorSubSel(gid) );
}


LineHorSubSelSet::LineHorSubSelSet( GeomID gid, trcnr_type tnr )
{
    add( new LineHorSubSel(gid,tnr) );
}


bool LineHorSubSelSet::operator ==( const LineHorSubSelSet& oth ) const
{
    const auto sz = size();
    if ( sz != oth.size() )
	return false;

    for ( auto idx=0; idx<sz; idx++ )
    {
	const auto& mylhss = *get( idx );
	const auto* othlhss = oth.find( mylhss.geomID() );
	if ( !othlhss || *othlhss != mylhss )
	    return false;
    }

    return true;
}


bool LineHorSubSelSet::includes( const LineHorSubSelSet& oth ) const
{
    const auto sz = size();
    if ( sz < oth.size() )
	return false;

    for ( auto idx=0; idx<sz; idx++ )
    {
	const auto& mylhss = *get( idx );
	const auto* othlhss = oth.find( mylhss.geomID() );
	if ( othlhss && !mylhss.includes(*othlhss) )
	    return false;
    }

    return true;
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


void LineHorSubSelSet::clearSubSel()
{
    for ( auto lhss : *this )
	lhss->clearSubSel();
}


LineHorSubSelSet::totalsz_type
LineHorSubSelSet::globIdx( const Bin2D& b2d ) const
{
    totalsz_type totsz = 0;
    for ( auto lhss : *this )
    {
	if ( lhss->geomID() == b2d.geomID() )
	    return totsz + lhss->globIdx( b2d.trcNr() );
	totsz += lhss->totalSize();
    }
    return -1;
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


CubeHorSubSel::CubeHorSubSel( const SurveyInfo* si )
    : CubeHorSubSel( SI(si).inlRange(), SI(si).crlRange() )
{
    si_ = si;
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


CubeHorSubSel::CubeHorSubSel( const CubeHorSubSel& oth )
    : Pos::IdxSubSel2D( oth.inlRange(), oth.crlRange() )
{
    si_ = oth.si_;
}


CubeHorSubSel::CubeHorSubSel( const CubeHorSubSel& org, int nrchunks,
							int chunknr )
    : Pos::IdxSubSel2D( org.inlRange(), org.crlRange() )
{
    if ( nrchunks < 2 )
	return;

    const auto orgnrinl = nrInl();
    auto chunksz = orgnrinl / nrchunks;
    if ( chunksz < 1 )
	chunksz = 1;

    auto startidx = chunksz * chunknr;
    if ( startidx >= orgnrinl )
	startidx = orgnrinl-1;
    if ( startidx < 0 )
	return;

    auto stopidx = startidx + chunksz - 1;
    if ( chunknr == nrchunks-1 )
	stopidx = orgnrinl - 1;

    pos_steprg_type inlrg;
    inlrg.start = inl4Idx( startidx );
    inlrg.stop = inl4Idx( stopidx );
    inlrg.step = inlSubSel().posStep();
    setInlRange( inlrg );
}


bool CubeHorSubSel::equals( const SubSel& ss ) const
{
    mDynamicCastGet( const CubeHorSubSel*, oth, &ss );
    if ( !oth )
	return false;

    return ssdata0_.sameOutputPosRange( oth->ssdata0_ ) &&
	   ssdata1_.sameOutputPosRange( oth->ssdata1_ );
}


Pos::GeomID CubeHorSubSel::geomID() const
{
    return GeomID::get3D();
}


CubeHorSubSel::totalsz_type CubeHorSubSel::totalSize() const
{
    return ((totalsz_type)nrInl()) * nrCrl();
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


bool CubeHorSubSel::getIntersection( const CubeHorSubSel& oth,
				     CubeHorSubSel& out ) const
{
    CubeHorSubSel chss1( *this ); chss1.normalise();
    CubeHorSubSel chss2( oth );   chss2.normalise();

    const pos_steprg_type inlrg1( chss1.inlRange() );
    const pos_steprg_type inlrg2( chss2.inlRange() );
    const pos_steprg_type crlrg1( chss1.crlRange() );
    const pos_steprg_type crlrg2( chss2.crlRange() );
    pos_steprg_type commoninlrg, commoncrlrg;

    const bool success = Pos::intersect(inlrg1,inlrg2,commoninlrg) &&
			 Pos::intersect(crlrg1,crlrg2,commoncrlrg);
    if ( success )
    {
	out.setInlRange( commoninlrg );
	out.setCrlRange( commoncrlrg );
    }
    return success;
}


void CubeHorSubSel::normalise()
{
    Pos::steprg_type inlrg( inlRange() ), crlrg( crlRange() );
    Pos::normalise( inlrg, SI().inlStep() );
    Pos::normalise( crlrg, SI().crlStep() );
    setInlRange( inlrg );
    setCrlRange( crlrg );
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


bool CubeHorSubSel::doUseJSON( const OD::JSON::Object& obj )
{
    auto rg( inlRange() );
    if ( obj.get(sKey::InlRange(),rg) )
	setInlRange( rg );
    rg = crlRange();
    if ( obj.get(sKey::CrlRange(),rg) )
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


void CubeHorSubSel::doFillJSON( OD::JSON::Object& obj ) const
{
    obj.set( sKey::InlRange(), inlRange() );
    obj.set( sKey::CrlRange(), crlRange() );
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


bool LineSubSel::equals( const SubSel& ss ) const
{
    mDynamicCastGet( const LineSubSel*, oth, &ss );
    if ( !oth )
	return false;

    return hss_ == oth->hss_ && zss_.sameOutputPosRange( oth->zss_ );
}


bool LineSubSel::includes( const LineSubSel& oth ) const
{
    return hss_.includes( oth.hss_ ) && zss_.includes( oth.zss_ );
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


bool LineSubSel::getIntersection( const LineSubSel& oth, LineSubSel& out ) const
{
    if ( !hss_.getIntersection(oth.hss_,out.hss_) )
	return false;

    ZSampling zsamp1( zRange() );	Pos::normaliseZ( zsamp1 );
    ZSampling zsamp2( oth.zRange() );	Pos::normaliseZ( zsamp2 );
    ZSampling zout;
    const bool success = Pos::intersectF( zsamp1, zsamp2, zout );
    if ( success )
	out.setZRange( zout );
    return success;
}


void LineSubSel::normalise()
{
    hss_.normalise();
    ZSampling zsamp( zRange() );
    Pos::normaliseZ( zsamp );
    setZRange( zsamp );
}



LineSubSelSet::LineSubSelSet( const IOPar& par )
{
    const Survey::FullSubSel fss( par );
    *this = fss.lineSubSelSet();
}


LineSubSelSet::LineSubSelSet( const LineHorSubSelSet& lhsss )
{
    for ( auto lhss : lhsss )
	add( new LineSubSel( *lhss ) );
}


bool LineSubSelSet::operator ==( const LineSubSelSet& oth ) const
{
    const auto sz = size();
    if ( sz != oth.size() )
	return false;

    for ( auto idx=0; idx<sz; idx++ )
    {
	const auto& mylss = *get( idx );
	const auto* othlss = oth.find( mylss.geomID() );
	if ( !othlss || *othlss != mylss )
	    return false;
    }

    return true;
}


bool LineSubSelSet::includes( const LineSubSelSet& oth ) const
{
    const auto sz = size();
    if ( sz < oth.size() )
	return false;

    for ( auto idx=0; idx<sz; idx++ )
    {
	const auto& mylss = *get( idx );
	const auto* othlss = oth.find( mylss.geomID() );
	if ( othlss && !mylss.includes(*othlss) )
	    return false;
    }

    return true;
}


void LineSubSelSet::setToAll()
{
    setEmpty();
    GeomIDSet allgids;
    SurvGeom2D::getGeomIDs( allgids );
    for ( auto gid : allgids )
	add( new LineSubSel(gid) );
}


void LineSubSelSet::clearSubSel()
{
    for ( auto lss : *this )
	lss->clearSubSel();
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


void LineSubSelSet::fillPar( IOPar& par ) const
{
    const Survey::FullSubSel fss( *this );
    fss.fillPar( par );
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


void LineSubSelSet::limitTo( const GeomIDSet& gids )
{
    for ( int idx=size()-1; idx>=0; idx-- )
	if ( !gids.isPresent(get(idx)->geomID()) )
	    removeSingle( idx );
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


CubeSubSel::CubeSubSel( const SurveyInfo* si )
    : GeomSubSel(SI(si).zRange())
    , hss_(si)
{
}


CubeSubSel::CubeSubSel( const Geometry3D& geom )
    : Survey::GeomSubSel( geom.zRange() )
    , hss_( geom )
{
}


CubeSubSel::CubeSubSel( const CubeHorSubSel& hss )
    : CubeSubSel( hss, hss.survInfo().zRange() )
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


CubeSubSel::CubeSubSel( const CubeSubSel& org, int nrchunks, int chunknr )
    : Survey::GeomSubSel(org.zss_.zRange())
    , hss_(org.hss_,nrchunks,chunknr)
{
}


bool CubeSubSel::equals( const SubSel& ss ) const
{
    mDynamicCastGet( const CubeSubSel*, oth, &ss );
    if ( !oth )
	return false;

    return hss_ == oth->hss_ && zss_.sameOutputPosRange( oth->zss_ );
}


bool CubeSubSel::includes( const CubeSubSel& oth ) const
{
    return hss_.includes( oth.hss_ ) && zss_.includes( oth.zss_ );
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


bool CubeSubSel::getIntersection( const CubeSubSel& oth,
				  CubeSubSel& out ) const
{
    if ( !hss_.getIntersection(oth.hss_,out.hss_) )
	return false;

    ZSampling zsamp1( zRange() );	Pos::normaliseZ( zsamp1 );
    ZSampling zsamp2( oth.zRange() );	Pos::normaliseZ( zsamp2 );
    ZSampling zout;
    const bool success = Pos::intersectF( zsamp1, zsamp2, zout );
    if ( success )
	out.setZRange( zout );
    return success;
}


void CubeSubSel::normalise()
{
    hss_.normalise();
    ZSampling zsamp( zRange() );
    Pos::normaliseZ( zsamp );
    setZRange( zsamp );
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
