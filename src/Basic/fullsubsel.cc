/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : somewhere around 1999
-*/


#include "fullsubsel.h"
#include "cubesubsel.h"
#include "linesubsel.h"
#include "trckeyzsampling.h"
#include "uistrings.h"


mUseType( Survey::FullSubSel, pos_type );
mUseType( Survey::FullSubSel, pos_rg_type );
mUseType( Survey::FullSubSel, idx_type );
mUseType( Survey::FullSubSel, size_type );
typedef StepInterval<pos_type> pos_steprg_type;
mUseType( Survey::FullSubSel, z_type );
mUseType( Survey::FullSubSel, z_rg_type );
mUseType( Survey::FullSubSel, z_steprg_type );


void Survey::FullSubSel::setZSSIfNotPresent( GeomID gid )
{
    if ( !gid.isValid() )
	return;

    const auto idx = indexOf( gid );
    if ( idx < 0 )
	zss_.setFull( gid );
}


void Survey::FullSubSel::fillFullZSS()
{
    for ( int idx=0; idx<hss_.nrGeomIDs(); idx++ )
	zss_.setFull( hss_.geomID(idx) );
}


void Survey::FullSubSel::setFromZSS( const FullZSubSel& inpfzss )
{
    for ( auto idx=0; idx<inpfzss.size(); idx++ )
    {
	const auto gid = inpfzss.geomID( idx );
	const auto idxof = hss_.indexOf( gid );
	if ( idxof<0 )
	    hss_.setFull( gid ); // adds missing line
	else
	    zss_.get( idxof ) = inpfzss.get( idx ); // sets to correct zss
    }
}


Survey::FullSubSel::FullSubSel( const SurveyInfo* si )
    : hss_(si)
    , zss_(si)
{
}


Survey::FullSubSel::FullSubSel( GeomID gid, const SurveyInfo* si )
    : FullSubSel(GeomIDSet(gid),si)
{
}


Survey::FullSubSel::FullSubSel( const GeomIDSet& gids, const SurveyInfo* si )
    : hss_(gids,si)
{
    zss_.setEmpty();
    for ( auto gid : gids )
	zss_.setFull( gid, &hss_.survInfo() );
}



Survey::FullSubSel::FullSubSel( const CubeSubSel& css )
    : hss_(css)
{
    zss_.first() = css.zSubSel();
}


Survey::FullSubSel::FullSubSel( const LineSubSel& lss )
    : hss_(lss)
{
    zss_.first() = lss.zSubSel();
}


Survey::FullSubSel::FullSubSel( const GeomSubSel& gss )
    : hss_(gss)
{
    fillFullZSS();
}


Survey::FullSubSel::FullSubSel( const FullHorSubSel& hss )
    : hss_(hss)
{
    fillFullZSS();
}


Survey::FullSubSel::FullSubSel( const FullHorSubSel& hss,
				const FullZSubSel& inpfzss )
    : hss_(hss)
{
    fillFullZSS();
    setFromZSS( inpfzss );
}


Survey::FullSubSel::FullSubSel( const FullZSubSel& inpfzss )
    : hss_(inpfzss.isEmpty() ? GeomID() : inpfzss.firstGeomID())
{
    setFromZSS( inpfzss );
}


Survey::FullSubSel::FullSubSel( const CubeHorSubSel& chss )
    : hss_(chss)
{
    fillFullZSS();
}


Survey::FullSubSel::FullSubSel( const LineHorSubSel& lhss )
    : hss_(lhss)
{
    fillFullZSS();
}


Survey::FullSubSel::FullSubSel( const LineSubSelSet& lsss )
    : hss_(lsss)
{
    zss_.setEmpty();
    for ( auto lss : lsss )
	zss_.set( lss->geomID(), lss->zSubSel() );
}


Survey::FullSubSel::FullSubSel( const LineHorSubSelSet& lhsss )
    : hss_(lhsss)
{
    fillFullZSS();
}


Survey::FullSubSel::FullSubSel( const BinID& bid )
    : FullSubSel( TrcKey(bid) )
{}
Survey::FullSubSel::FullSubSel( GeomID gid, trcnr_type tnr )
    : FullSubSel( TrcKey(gid,tnr) )
{}
Survey::FullSubSel::FullSubSel( const TrcKey& tk )
    : hss_(tk)
{
    fillFullZSS();
}


Survey::FullSubSel::FullSubSel( const TrcKeySampling& tks )
    : hss_(tks)
{
    fillFullZSS();
}


Survey::FullSubSel::FullSubSel( const TrcKeyZSampling& tkzs )
    : hss_(tkzs)
{
    fillFullZSS();
    for ( auto idx=0; idx<zss_.size(); idx++ )
	zss_.get(idx) = ZSubSel( tkzs.zsamp_ );
}


Survey::FullSubSel::FullSubSel( const FullSubSel& oth )
    : hss_(oth.hss_)
    , zss_(oth.zss_)
{
}


Survey::FullSubSel::FullSubSel( const IOPar& iop )
    : FullSubSel()
{
    usePar( iop );
}


Survey::FullSubSel::~FullSubSel()
{
}


Survey::FullSubSel& Survey::FullSubSel::operator=( const FullSubSel& oth )
{
    if ( this != &oth )
    {
	hss_ = oth.hss_;
	zss_ = oth.zss_;
    }
    return *this;
}


bool Survey::FullSubSel::operator==( const FullSubSel& oth ) const
{
    return hss_ == oth.hss_ && zss_ == oth.zss_;
}


void Survey::FullSubSel::setZSubSel( GeomID gid, const ZSubSel& zss )
{
    zss_.set( gid, zss );
    auto idxof = hss_.indexOf( gid );
    if ( idxof < 0 )
	hss_.addGeomID( gid );
}


CubeHorSubSel Survey::FullSubSel::cubeHorSubSel() const
{
    return hss_.cubeHorSubSel();
}


CubeSubSel Survey::FullSubSel::cubeSubSel() const
{
    CubeSubSel ret( hss_.cubeHorSubSel() );
    ret.zSubSel() = zss_.first();
    return ret;
}


LineHorSubSel Survey::FullSubSel::lineHorSubSel( idx_type iln ) const
{
    return hss_.lineHorSubSel( iln );
}


LineSubSel Survey::FullSubSel::lineSubSel( idx_type iln ) const
{
    LineSubSel ret( hss_.lineHorSubSel(iln) );
    ret.zSubSel() = zss_.getFor( ret.geomID() );
    return ret;
}


void Survey::FullSubSel::getLineHorSubSelSet( LineHorSubSelSet& lhsss ) const
{
    lhsss.setEmpty();
    for ( int idx=0; idx<nrGeomIDs(); idx++ )
	lhsss.add( new LineHorSubSel( hss_.lineHorSubSel(idx) ) );
}


LineHorSubSelSet Survey::FullSubSel::lineHorSubSelSet() const
{
    LineHorSubSelSet ret;
    getLineHorSubSelSet( ret );
    return ret;
}


void Survey::FullSubSel::getLineSubSelSet( LineSubSelSet& lsss ) const
{
    lsss.setEmpty();
    for ( int idx=0; idx<nrGeomIDs(); idx++ )
    {
	auto* lss = new LineSubSel( hss_.lineHorSubSel(idx) );
	lss->zSubSel() = zss_.getFor( lss->geomID() );
	lsss.add( lss );
    }
}


LineSubSelSet Survey::FullSubSel::lineSubSelSet() const
{
    LineSubSelSet ret;
    getLineSubSelSet( ret );
    return ret;
}


Survey::GeomSubSel* Survey::FullSubSel::getGeomSubSel( idx_type idx ) const
{
    if ( is2D() )
	return new LineSubSel( lineSubSel(idx) );
    else
	return new CubeSubSel( cubeSubSel() );
}


void Survey::FullSubSel::merge( const FullSubSel& oth )
{
    hss_.merge( oth.hss_ );
    zss_.merge( oth.zss_ );
}


void Survey::FullSubSel::limitTo( const FullSubSel& oth )
{
    hss_.limitTo( oth.hss_ );
    zss_.limitTo( oth.zss_ );
}


bool Survey::FullSubSel::isAll() const
{
    return hss_.isAll() && zss_.isAll();
}


bool Survey::FullSubSel::isFlat() const
{
    if ( is2D() || isZSlice() )
	return true;

    const auto& chss = hss_.cubeHorSubSel();
    return chss.nrInl() == 1 || chss.nrCrl() == 1;
}


bool Survey::FullSubSel::isZSlice() const
{
    return zss_.first().size() == 1;
}


void Survey::FullSubSel::setEmpty()
{
    hss_.setEmpty();
    zss_.setEmpty();
}


void Survey::FullSubSel::setToAll( bool is2d )
{
    hss_.setToAll( is2d );
    zss_.setEmpty();
    fillFullZSS();
}


void Survey::FullSubSel::setFull( GeomID gid )
{
    hss_.setFull( gid );
    zss_.setFull( gid );
}


void Survey::FullSubSel::syncZSS()
{
    for ( auto idx=0; idx<hss_.nrGeomIDs(); idx++ )
	setZSSIfNotPresent( hss_.geomID(idx) );
    GeomIDSet gids2remove;
    for ( auto idx=0; idx<zss_.nrGeomIDs(); idx++ )
    {
	const auto gid = zss_.geomID( idx );
	if ( !hss_.isPresent(gid) )
	    gids2remove.add( gid );
    }
    for ( auto gid : gids2remove )
	zss_.remove( gid );
}


void Survey::FullSubSel::set( const CubeHorSubSel& css )
{
    hss_ = FullHorSubSel( css );
    syncZSS();
}


void Survey::FullSubSel::set( const LineHorSubSel& lss )
{
    hss_ = FullHorSubSel( lss );
    syncZSS();
}


void Survey::FullSubSel::set( const LineHorSubSelSet& lsss )
{
    hss_ = FullHorSubSel( lsss );
    syncZSS();
}


void Survey::FullSubSel::set( const CubeSubSel& css )
{
    *this = FullSubSel( css );
}


void Survey::FullSubSel::set( const LineSubSel& lss )
{
    *this = FullSubSel( lss );
}


void Survey::FullSubSel::set( const LineSubSelSet& lsss )
{
    *this = FullSubSel( lsss );
}


void Survey::FullSubSel::set( const GeomSubSel& gss )
{
    *this = FullSubSel( gss );
}


void Survey::FullSubSel::setGeomID( GeomID gid )
{
    if ( !gid.isValid() )
	return;

    hss_.setGeomID( gid );
    setZSSIfNotPresent( gid );
    const auto kpzss = zSubSel( gid );
    zss_.setEmpty();
    zss_.set( gid, kpzss );
}


void Survey::FullSubSel::addGeomID( GeomID gid )
{
    const auto oldnrgeomids = hss_.nrGeomIDs();
    hss_.addGeomID( gid );
    if ( oldnrgeomids < hss_.nrGeomIDs() )
	zss_.setFull( gid );
}


uiString Survey::FullSubSel::getUserSummary() const
{
    uiString ret( hss_.getUserSummary() );
    ret.appendPhrase( zss_.getUserSummary() );
    return ret;
}


void Survey::FullSubSel::fillPar( IOPar& iop ) const
{
    hss_.fillPar( iop );
    zss_.fillPar( iop );
}


void Survey::FullSubSel::usePar( const IOPar& iop, const SurveyInfo* si )
{
    hss_.usePar( iop, si );
    zss_.usePar( iop, si );
    syncZSS();
}
