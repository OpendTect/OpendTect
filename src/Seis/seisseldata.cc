/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-1-1998 / Mar 2019
-*/


#include "seispolyseldata.h"
#include "seisrangeseldata.h"
#include "seistableseldata.h"
#include "cubesubsel.h"
#include "iopar.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "picksettr.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "posvecdatasettr.h"
#include "seis2ddata.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "trckey.h"

static const char* sKeyBinIDSel = "BinID selection";
mUseType( Seis::SelData, z_type );
mUseType( Seis::SelData, z_steprg_type );
mUseType( Seis::SelData, pos_type );
mUseType( Seis::SelData, pos_rg_type );
mUseType( Seis::SelData, idx_type );
mUseType( Seis::SelData, size_type );
typedef StepInterval<pos_type> pos_steprg_type;
typedef StepInterval<z_type> z_steprg_type;


bool Seis::SelData::operator ==( const Seis::SelData& oth ) const
{
    if ( &oth == this )
	return true;

    if ( type() != oth.type() || is2D() != oth.is2D() ||
	 isAll() != oth.isAll() )
	return false;

    IOPar thisiop, othiop;
    fillPar( thisiop );
    oth.fillPar( othiop );

    return thisiop == othiop;
}


bool Seis::SelData::operator !=( const Seis::SelData& oth ) const
{
    return !(*this == oth);
}


const char* Seis::SelData::sNrLinesKey()
{
    return Survey::FullHorSubSel::sNrLinesKey();
}


Seis::SelDataPosIter::SelDataPosIter( const SelData& sd )
    : sd_(sd)
{
}


Seis::SelDataPosIter::SelDataPosIter( const SelDataPosIter& oth )
    : sd_(oth.sd_)
{
}


pos_type Seis::SelDataPosIter::trcNr() const
{
    return binID().crl();
}


void Seis::SelDataPosIter::getTrcKey( TrcKey& tk ) const
{
    if ( is2D() )
	tk.setPos( geomID(), trcNr() );
    else
	tk.setPos( binID() );
}


void Seis::SelData::copyFrom( const SelData& oth )
{
    if ( this != &oth )
	doCopyFrom( oth );
}


void Seis::SelData::removeFromPar( IOPar& iop, const char* subky )
{
    iop.removeWithKey( IOPar::compKey(subky,sKey::Type()) );
    iop.removeWithKey( IOPar::compKey(subky,sKeyBinIDSel) );
}


Seis::SelData* Seis::SelData::get( Type t )
{
    switch ( t )
    {
    case Table:		return new TableSelData;
    case Polygon:	return new PolySelData;
    default:		return new RangeSelData( GeomID::get3D() );
    }
}



Seis::SelData* Seis::SelData::get( const IOPar& iop, const SurveyInfo* si )
{
    const Type t = selTypeOf( iop.find(sKey::Type()) );
    SelData* sd = get( t );
    sd->usePar( iop, si );
    if ( sd->asRange() && sd->asRange()->fullSubSel().nrGeomIDs() < 1 )
	deleteAndZeroPtr(sd);
    return sd;
}


Seis::SelData* Seis::SelData::get( const DBKey& dbky )
{
    PtrMan<IOObj> ioobj = getIOObj( dbky );
    if ( !ioobj )
	return nullptr;

    const SeisIOObjInfo objinf( *ioobj );
    if ( objinf.isOK() )
    {
	PtrMan<Survey::FullSubSel> ss = objinf.getSurvSubSel();
	return new RangeSelData( *ss );
    }

    if ( ioobj->group() == mTranslGroupName(PosVecDataSet) )
	return new TableSelData( ioobj->key() );
    else if ( ioobj->group() == mTranslGroupName(PickSet) )
    {
	if ( PickSetTranslator::isPolygon(*ioobj) )
	    return new PolySelData( ioobj->key() );
	else
	    return new TableSelData( ioobj->key() );
    }

    pFreeFnErrMsg( BufferString("No Seis::SelData for ",ioobj->group()) );
    return nullptr;
}


bool Seis::SelData::isOK( const TrcKey& tk ) const
{
    return tk.is2D() ? isOK( tk.geomID(), tk.trcNr() )
		     : isOK( tk.position() );
}


bool Seis::SelData::isOK( const IdxPair& ip ) const
{
    return is2D() ? isOK( GeomID(ip.lineNr()), ip.trcNr() )
		  : isOK( BinID(ip) );
}


void Seis::SelData::include( const SelData& oth )
{
    if ( oth.type() != type() )
	{ pErrMsg("Cannot include mixed types"); }
    else if ( isRange() )
	asRange()->merge( *oth.asRange() );
    else if ( isTable() )
	asTable()->merge( *oth.asTable() );
    else if ( isRange() )
	asPoly()->merge( *oth.asPoly() );
}


Seis::SelData::idx_type Seis::SelData::indexOf( GeomID gid ) const
{
    for ( idx_type igid=0; igid<nrGeomIDs(); igid++ )
	if ( geomID(igid) == gid )
	    return igid;
    return -1;
}


Seis::RangeSelData* Seis::SelData::asRange()
{
    return isRange() ? static_cast<RangeSelData*>( this ) : nullptr;
}


const Seis::RangeSelData* Seis::SelData::asRange() const
{
    return isRange() ? static_cast<const RangeSelData*>( this ) : nullptr;
}


Seis::TableSelData* Seis::SelData::asTable()
{
    return isTable() ? static_cast<TableSelData*>( this ) : nullptr;
}


const Seis::TableSelData* Seis::SelData::asTable() const
{
    return isTable() ? static_cast<const TableSelData*>( this ) : nullptr;
}


Seis::PolySelData* Seis::SelData::asPoly()
{
    return isPoly() ? static_cast<PolySelData*>( this ) : nullptr;
}


const Seis::PolySelData* Seis::SelData::asPoly() const
{
    return isPoly() ? static_cast<const PolySelData*>( this ) : nullptr;
}


static Seis::SelData::pos_type getInlCrl42D( const SurvGeom2D& geom,
					     bool first, bool inl )
{
    const Coord coord( geom.getCoordByIdx(first? 0 : geom.size()-1) );
    const BinID bid( SI().transform(coord) );
    return inl ? bid.inl() : bid.crl();
}


static void inclInlCrl42D( const SurvGeom2D& geom,
		    Seis::SelData::pos_rg_type& rg, bool inl )
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


Seis::SelData::pos_rg_type Seis::SelData::inlRange() const
{
    if ( !is2D() )
	return SI().inlRange();

    pos_rg_type ret( mUdf(int), mUdf(int) );
    for ( auto idx=0; idx<nrGeomIDs(); idx++ )
	inclInlCrl42D( SurvGeom2D::get( geomID(idx) ), ret, true );

    return ret;
}


Seis::SelData::pos_rg_type Seis::SelData::crlRange() const
{
    if ( !is2D() )
	return SI().crlRange();

    pos_rg_type ret( mUdf(int), mUdf(int) );
    for ( auto idx=0; idx<nrGeomIDs(); idx++ )
	inclInlCrl42D( SurvGeom2D::get( geomID(idx) ), ret, false );

    return ret;
}


Seis::SelData::z_steprg_type Seis::SelData::zRange( idx_type idx ) const
{
    return Survey::Geometry::get(geomID(idx)).zRange();
}



void Seis::SelData::fillPar( IOPar& iop ) const
{
    const char* typstr = Seis::nameOf(type());
    iop.set( sKey::Type(), isAll() ? (const char*)sKey::None() : typstr );

    if ( !is2D() )
	iop.removeWithKey( sNrLinesKey() );
    const auto nrgeomids = nrGeomIDs();
    if ( nrgeomids > 1 || !is2D() )
	iop.removeWithKey( sKey::GeomID() );

    if ( is2D() )
    {
	iop.set( sNrLinesKey(), nrgeomids );
	if ( nrgeomids == 1 )
	    iop.set( sKey::GeomID(), geomID() );
	else
	{
	    GeomIDSet gids;
	    for ( auto idx=0; idx<nrgeomids; idx++ )
		gids += geomID( idx );
	    iop.set( sKey::GeomID(mPlural), gids );
	}
    }

    doFillPar( iop );
}


void Seis::SelData::usePar( const IOPar& iop, const SurveyInfo* si )
{
    doUsePar( iop, si );
}


size_type Seis::SelData::nrTrcsInSI() const
{
    CubeSubSel css;
    return css.size( OD::InlineSlice ) * css.size( OD::CrosslineSlice );
}


uiString Seis::SelData::usrSummary() const
{
    return isAll() ? toUiString( "-" ) : gtUsrSummary();
}


// This default returns selection on base of position
// It will be called if the class does not support 2D directly

int Seis::SelData::selRes2D( GeomID gid, pos_type trcnr ) const
{
    if ( !gid.isValid() || !gid.is2D() )
	return 256+2;

    auto& geom = SurvGeom2D::get( gid );
    if ( geom.isEmpty() )
	return 256+2;

    const auto idxof = geom.indexOf( trcnr );
    if ( idxof < 0 )
	return 256;

    const BinID bid( SI().transform(geom.getCoordByIdx(idxof)) );
    return selRes3D( bid );
}


int Seis::SelData::selRes( const Bin2D& b2d ) const
{
    return selRes2D( b2d.geomID(), b2d.trcNr() );
}


BinnedValueSet* Seis::SelData::applyTo( const LineCollData& lcd ) const
{
    const bool is2d = is2D();
    auto* ret = new BinnedValueSet( 0, false,
			is2D() ? OD::LineBasedGeom : OD::VolBasedGeom );

    for ( int iln=0; iln<lcd.size(); iln++ )
    {
	const auto& ld = *lcd.get( iln );
	if ( ld.isEmpty() )
	    { pErrMsg("Empty linedata"); continue; }
	auto selres = is2d ? selRes( ld.firstBin2D() )
			   : selRes( ld.firstBinID() );
	if ( (selres%256) == 2 )
	    continue;

	PosInfo::LineDataPos ldp;
	while ( ld.toNext( ldp ) )
	{
	    if ( is2d )
	    {
		const Bin2D b2d( GeomID(ld.linenr_), ld.pos(ldp) );
		if ( isOK(b2d) )
		    ret->add( b2d );
	    }
	    else
	    {
		const BinID bid( ld.linenr_, ld.pos(ldp) );
		if ( isOK(bid) )
		    ret->add( bid );
	    }
	}
    }

    return ret;
}
