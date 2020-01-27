/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-1-1998 / Mar 2019
-*/


#include "seisrangeseldata.h"
#include "cubesubsel.h"
#include "iopar.h"
#include "linesubsel.h"
#include "survinfo.h"

mUseType( Seis::SelData, z_type );
mUseType( Seis::SelData, z_steprg_type );
mUseType( Seis::SelData, pos_type );
mUseType( Seis::SelData, pos_rg_type );
mUseType( Seis::SelData, idx_type );
mUseType( Seis::SelData, size_type );
typedef StepInterval<z_type> z_steprg_type;
typedef StepInterval<pos_type> pos_steprg_type;



Seis::RangeSelData::RangeSelData( const SurveyInfo* si )
    : fss_(si)
{}
Seis::RangeSelData::RangeSelData( GeomID gid, const SurveyInfo* si )
    : fss_(gid,si)
{}
Seis::RangeSelData::RangeSelData( const GeomIDSet& gids, const SurveyInfo* si )
    : fss_(gids,si)
{}
Seis::RangeSelData::RangeSelData( const BinID& bid )
    : fss_(bid)
{}
Seis::RangeSelData::RangeSelData( GeomID gid, trcnr_type tnr )
    : fss_(gid,tnr)
{}
Seis::RangeSelData::RangeSelData( const TrcKey& tk )
    : fss_(tk)
{}
Seis::RangeSelData::RangeSelData( const CubeSubSel& css )
    : fss_(css)
{}
Seis::RangeSelData::RangeSelData( const LineSubSel& lss )
    : fss_(lss)
{}
Seis::RangeSelData::RangeSelData( const GeomSubSel& gss )
    : fss_(gss)
{}
Seis::RangeSelData::RangeSelData( const CubeHorSubSel& chss )
    : fss_(chss)
{}
Seis::RangeSelData::RangeSelData( const LineHorSubSel& lhss )
    : fss_(lhss)
{}
Seis::RangeSelData::RangeSelData( const LineSubSelSet& lsss )
    : fss_(lsss)
{}
Seis::RangeSelData::RangeSelData( const LineHorSubSelSet& lhsss )
    : fss_(lhsss)
{}
Seis::RangeSelData::RangeSelData( const FullSubSel& fss )
    : fss_(fss)
{}
Seis::RangeSelData::RangeSelData( const TrcKeySampling& tks )
    : fss_(tks)
{}
Seis::RangeSelData::RangeSelData( const TrcKeyZSampling& tkzs )
    : fss_(tkzs)
{}
Seis::RangeSelData::RangeSelData( const RangeSelData& oth )
    : RangeSelData()
{ copyFrom( oth ); }
Seis::RangeSelData::RangeSelData( const IOPar& iop, const SurveyInfo* si )
    : RangeSelData()
{ usePar( iop, si ); }


Seis::RangeSelData::~RangeSelData()
{
}


size_type Seis::RangeSelData::nrGeomIDs() const
{
    return fss_.nrGeomIDs();
}


Seis::SelDataPosIter* Seis::RangeSelData::posIter() const
{
    return new RangeSelDataPosIter( *this );
}


void Seis::RangeSelData::clearContents()
{
    fss_.setEmpty();
}


void Seis::RangeSelData::doCopyFrom( const SelData& sd )
{
    clearContents();

    if ( sd.type() == type() )
    {
	mDynamicCastGet(const RangeSelData&,oth,sd)
	fss_ = oth.fss_;
	forceall_ = oth.forceall_;
    }
    else if ( sd.is2D() )
    {
	const auto nrgids = sd.nrGeomIDs();
	LineSubSelSet lsss;
	for ( auto idx=0; idx<nrgids; idx++ )
	{
	    auto* lss = new LineSubSel( sd.geomID(idx) );
	    lss->setTrcNrRange( pos_steprg_type(sd.trcNrRange(idx)) );
	    lss->setZRange( sd.zRange(idx) );
	    lsss.add( lss );
	}
	fss_.set( lsss );
    }
    else
    {
	CubeSubSel css;
	css.setInlRange( pos_steprg_type(sd.inlRange(),SI().inlStep()) );
	css.setCrlRange( pos_steprg_type(sd.crlRange(),SI().crlStep()) );
	css.setZRange( sd.zRange() );
	fss_.set( css );
    }
}


void Seis::RangeSelData::merge( const RangeSelData& oth )
{
    if ( oth.forceall_ )
	forceall_ = true;
    fss_.merge( oth.fss_ );
}


bool Seis::RangeSelData::isAll() const
{
    if ( forceall_ )
	return true;
    return fss_.isAll();
}


void Seis::RangeSelData::setToAll()
{
    fss_.setToAll( is2D() );
}


void Seis::RangeSelData::setToAll( bool make2d )
{
    fss_.setToAll( make2d );
}


pos_rg_type Seis::RangeSelData::inlRange() const
{
    if ( forceall_ )
    {
	CubeSubSel css;
	return css.fullInlRange();
    }
    return fss_.inlRange();
}


pos_rg_type Seis::RangeSelData::crlRange() const
{
    if ( forceall_ )
    {
	CubeSubSel css;
	return css.fullCrlRange();
    }
    return fss_.crlRange();
}


pos_rg_type Seis::RangeSelData::trcNrRange( idx_type iln ) const
{
    if ( !is2D() )
	return crlRange();

    if ( forceall_ )
	return LineHorSubSel( geomID(iln) ).fullTrcNrRange();

    return fss_.trcNrRange( iln );
}


z_steprg_type Seis::RangeSelData::zRange( idx_type iln ) const
{
    if ( !is2D() )
	return forceall_ ? SelData::zRange() : fss_.zRange();

    if ( forceall_ )
	return LineSubSel( geomID(iln) ).zSubSel().inputZRange();

    return fss_.zRange( iln );
}


z_steprg_type Seis::RangeSelData::zRangeFor( GeomID gid ) const
{
    const auto iln = fss_.indexOf( gid );
    if ( iln >= 0 )
	return zRange( iln );
    return SI().zRange();
}


Pos::GeomID Seis::RangeSelData::gtGeomID( idx_type iln ) const
{
    return fss_.geomID( iln );
}


idx_type Seis::RangeSelData::indexOf( GeomID gid ) const
{
    return fss_.indexOf( gid );
}


void Seis::RangeSelData::setInlRange( const pos_rg_type& rg )
{
    fss_.setInlRange( rg );
}


void Seis::RangeSelData::setCrlRange( const pos_rg_type& rg )
{
    fss_.setCrlRange( rg );
}


void Seis::RangeSelData::setTrcNrRange( GeomID gid, const pos_rg_type& rg )
{
    fss_.setTrcNrRange( gid, rg );
}


void Seis::RangeSelData::setTrcNrRange( const pos_rg_type& rg, idx_type idx )
{
    fss_.setTrcNrRange( rg, idx );
}


void Seis::RangeSelData::setZRange( const z_steprg_type& rg, idx_type idx )
{
    fss_.zSubSel(idx).setOutputZRange( rg );
}


void Seis::RangeSelData::set( const CubeSubSel& css )
{
    fss_.set( css );
}


void Seis::RangeSelData::set( const LineSubSel& lss )
{
    fss_.set( lss );
}


void Seis::RangeSelData::set( const LineSubSelSet& lsss )
{
    fss_.set( lsss );
}


void Seis::RangeSelData::setGeomID( GeomID gid )
{
    fss_.setGeomID( gid );
}


void Seis::RangeSelData::addGeomID( GeomID gid )
{
    fss_.addGeomID( gid );
}


void Seis::RangeSelData::doFillPar( IOPar& iop ) const
{
    if ( !forceall_ )
	fss_.fillPar( iop );
}


void Seis::RangeSelData::doUsePar( const IOPar& iop, const SurveyInfo* si )
{
    clearContents();
    fss_.usePar( iop, si );
}


int Seis::RangeSelData::selRes3D( const BinID& bid ) const
{
    return forceall_ ? 0 : fss_.fullHorSubSel().selRes3D( bid );
}


int Seis::RangeSelData::selRes2D( GeomID gid, pos_type trcnr ) const
{
    return forceall_ ? 0 : fss_.fullHorSubSel().selRes2D( gid, trcnr );
}


uiString Seis::RangeSelData::gtUsrSummary() const
{
    return fss_.getUserSummary();
}


size_type Seis::RangeSelData::expectedNrTraces() const
{
    return fss_.expectedNrPositions();
}


Seis::RangeSelDataPosIter::RangeSelDataPosIter( const RangeSelData& rsd )
    : SelDataPosIter(rsd)
    , iter_(rsd.fss_)
{
}


Seis::RangeSelDataPosIter::RangeSelDataPosIter( const RangeSelDataPosIter& oth )
    : SelDataPosIter(oth)
    , iter_(oth.iter_)
{
}
