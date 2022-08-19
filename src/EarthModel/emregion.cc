/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emregion.h"

#include "emfault3d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "faulttrace.h"
#include "ioman.h"
#include "uistrings.h"


namespace EM
{

// Region
Region::Region( Pos::GeomID geomid )
    : geomid_(geomid)
{
    tkzs_.init( false );
}


Region::~Region()
{}


int Region::id() const
{ return id_; }

const TrcKeyZSampling& Region::getBoundingBox() const
{ return tkzs_; }


bool Region::isInside( const TrcKey& tk, float z, bool ) const
{ return tkzs_.hsamp_.includes( tk ) && tkzs_.zsamp_.includes( z, false ); }


void Region::fillPar( IOPar& par ) const
{
}


bool Region::usePar( const IOPar& par )
{
    return true;
}


static const char* sSide()	{ return "Side"; }
static const char* sZ()		{ return "Z"; }

// RegionInlBoundary
const char* RegionInlBoundary::type() const
{ return sKey::Inline(); }


bool RegionInlBoundary::onRightSide( const TrcKey& tk, float z ) const
{
    return side_==0 ? tk.lineNr()>inl_ : tk.lineNr()<inl_;
}


void RegionInlBoundary::getSideStrs( uiStringSet& strs ) const
{
    strs.add( tr("Positive") ); strs.add( tr("Negative") );
}


void RegionInlBoundary::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), type() );
    par.set( sSide(), side_ );
    par.set( sKey::Inline(), inl_ );
}


bool RegionInlBoundary::usePar( const IOPar& par )
{
    par.get( sSide(), side_ );
    par.get( sKey::Inline(), inl_ );
    return true;
}


// RegionCrlBoundary
const char* RegionCrlBoundary::type() const
{ return sKey::Crossline(); }


bool RegionCrlBoundary::onRightSide( const TrcKey& tk, float z ) const
{
    return side_==0 ? tk.trcNr()>crl_ : tk.trcNr()<crl_;
}


void RegionCrlBoundary::getSideStrs( uiStringSet& strs ) const
{
    strs.add( tr("Positive") ); strs.add( tr("Negative") );
}


void RegionCrlBoundary::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), type() );
    par.set( sSide(), side_ );
    par.set( sKey::Crossline(), crl_ );
}


bool RegionCrlBoundary::usePar( const IOPar& par )
{
    par.get( sSide(), side_ );
    par.get( sKey::Crossline(), crl_ );
    return true;
}


// RegionZBoundary
const char* RegionZBoundary::type() const
{ return sKey::ZSlice(); }


bool RegionZBoundary::onRightSide( const TrcKey& tk, float z ) const
{
    return side_==0 ? z<z_ : z>z_;
}


void RegionZBoundary::getSideStrs( uiStringSet& strs ) const
{
    strs.add( uiStrings::sAbove() );
    strs.add( uiStrings::sBelow() );
}


void RegionZBoundary::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), type() );
    par.set( sSide(), side_ );
    par.set( sZ(), z_ );
}


bool RegionZBoundary::usePar( const IOPar& par )
{
    par.get( sSide(), side_ );
    par.get( sZ(), z_ );
    return true;
}


// RegionHor3DBoundary
RegionHor3DBoundary::RegionHor3DBoundary( const MultiID& mid )
    : RegionBoundary()
    , hor_(0)
{
    setKey( mid );
}


RegionHor3DBoundary::~RegionHor3DBoundary()
{
   if ( hor_ ) hor_->unRef();
}


const char* RegionHor3DBoundary::type() const
{ return sKey::Horizon(); }


void RegionHor3DBoundary::setKey( const MultiID& mid )
{
    key_ = mid;
    setName( IOM().nameOf(key_) );
}


bool RegionHor3DBoundary::init( TaskRunner* taskrunner )
{
    RefMan<EM::EMObject> emobj =
	EM::EMM().loadIfNotFullyLoaded( key_, taskrunner );
    mDynamicCast(EM::Horizon3D*,hor_,emobj.ptr())
    if ( hor_ ) hor_->ref();
    return hor_;
}


bool RegionHor3DBoundary::onRightSide( const TrcKey& tk, float z ) const
{
    if ( !hor_ ) return false;

    const float zattk = hor_->getZ( BinID(tk.lineNr(),tk.trcNr()) );
    if ( mIsUdf(zattk) ) return true;

    return side_==0 ? z < zattk : z > zattk;
}


void RegionHor3DBoundary::getSideStrs( uiStringSet& strs ) const
{
    strs.add( uiStrings::sAbove() );
    strs.add( uiStrings::sBelow() );
}


void RegionHor3DBoundary::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), type() );
    par.set( sSide(), side_ );
    par.set( sKey::ID(), key_ );
}


bool RegionHor3DBoundary::usePar( const IOPar& par )
{
    par.get( sSide(), side_ );

    MultiID mid = MultiID::udf();
    par.get( sKey::ID(), mid );
    setKey( mid );
    return true;
}


// RegionFaultBoundary
RegionFaultBoundary::RegionFaultBoundary( const MultiID& mid )
    : RegionBoundary()
    , flt_(0)
    , prov_(*new FaultTrcDataProvider)
{
    setKey( mid );
}


RegionFaultBoundary::~RegionFaultBoundary()
{
    delete &prov_;
    if ( flt_ )
	flt_->unRef();
}


const char* RegionFaultBoundary::type() const
{ return sKey::Fault(); }


void RegionFaultBoundary::setKey( const MultiID& mid )
{
    key_ = mid;
    setName( IOM().nameOf(key_) );
}


bool RegionFaultBoundary::init( TaskRunner* taskrunner )
{
    if ( flt_ ) flt_->unRef();
    RefMan<EM::EMObject> emobj =
	EM::EMM().loadIfNotFullyLoaded( key_, taskrunner );
    mDynamicCast(EM::Fault3D*,flt_,emobj.ptr())
    if ( flt_ ) flt_->ref();

    TypeSet<MultiID> keys;  keys.add( key_ );
    if ( !prov_.init(keys,TrcKeySampling(),taskrunner) )
	return false;

    return flt_;
}


bool RegionFaultBoundary::onRightSide( const TrcKey& tk, float z ) const
{
    const FaultTrace* inltrc = prov_.getFaultTrace( 0, tk.lineNr(), true );
    const FaultTrace* crltrc = prov_.getFaultTrace( 0, tk.trcNr(), false );
    if ( !inltrc && !crltrc )
	return false;

    const bool onposcrl =
		inltrc ? inltrc->isOnPosSide( tk.position(), z ) : false;
    const bool onposinl =
		crltrc ? crltrc->isOnPosSide( tk.position(), z ) : false;

    if ( side_==0 && onposinl ) return true;
    if ( side_==1 && onposcrl ) return true;
    if ( side_==2 && !onposinl ) return true;
    if ( side_==3 && !onposcrl ) return true;

    return false;
}


void RegionFaultBoundary::getSideStrs( uiStringSet& strs ) const
{
    strs.add(tr("Pos Inl"));
    strs.add(tr("Pos Crl"));
    strs.add(tr("Neg Inl"));
    strs.add(tr("Neg Crl"));

/*
    strs.add("Pos Inl/Crl");
    strs.add("Neg Inl/Pos Crl");
    strs.add("Neg Inl/Crl");
    strs.add("Pos Inl/Neg Crl");
*/
}


void RegionFaultBoundary::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), type() );
    par.set( sSide(), side_ );
    par.set( sKey::ID(), key_ );
}


bool RegionFaultBoundary::usePar( const IOPar& par )
{
    par.get( sSide(), side_ );

    MultiID mid = MultiID::udf();
    par.get( sKey::ID(), mid );
    setKey( mid );
    return true;
}


// RegionPolygonBoundary
RegionPolygonBoundary::RegionPolygonBoundary( const MultiID& mid )
    : RegionBoundary()
    , polygon_(0)
{
    setKey( mid );
}


RegionPolygonBoundary::~RegionPolygonBoundary()
{
    delete polygon_;
}


const char* RegionPolygonBoundary::type() const
{ return sKey::Polygon(); }


void RegionPolygonBoundary::setKey( const MultiID& mid )
{
    key_ = mid;
    setName( IOM().nameOf(key_) );
}


bool RegionPolygonBoundary::onRightSide( const TrcKey& tk, float z ) const
{
    if ( !polygon_ ) return false;

    const Geom::Point2D<float> pt( (float)tk.lineNr(), (float)tk.trcNr() );
    const bool isinside = polygon_->isInside( pt, false, 0.f );
    return side_ == 0 ? isinside : !isinside;
}


void RegionPolygonBoundary::getSideStrs( uiStringSet& strs ) const
{
    strs.add( tr("Inside") );
    strs.add( tr("Outside") );
}


void RegionPolygonBoundary::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), type() );
    par.set( sSide(), side_ );
    par.set( sKey::ID(), key_ );
}


bool RegionPolygonBoundary::usePar( const IOPar& par )
{
    par.get( sSide(), side_ );

    MultiID mid = MultiID::udf();
    par.get( sKey::ID(), mid );
    setKey( mid );

    return true;
}


// Region3D
Region3D::Region3D()
   : Region(Survey::default3DGeomID())
{}


Region3D::~Region3D()
{
    setEmpty();
}


void Region3D::addBoundary( RegionBoundary* bd )
{ boundaries_ += bd; }

RegionBoundary* Region3D::getBoundary( int idx )
{ return boundaries_.validIdx(idx) ? boundaries_[idx] : 0; }

void Region3D::removeBoundary( int idx )
{ delete boundaries_.removeSingle( idx ); }

void Region3D::removeBoundary( RegionBoundary& bd )
{ boundaries_ -= &bd; delete &bd; }

int Region3D::size() const
{ return boundaries_.size(); }

bool Region3D::isEmpty() const
{ return boundaries_.isEmpty(); }

void Region3D::setEmpty()
{ deepErase( boundaries_ ); }


bool Region3D::hasBoundary( const MultiID& mid ) const
{
    for ( int idx=0; idx<boundaries_.size(); idx++ )
    {
	const RegionBoundary* bd = boundaries_[idx];
	mDynamicCastGet(const RegionHor3DBoundary*,horbd,bd)
	if ( horbd && horbd->key()==mid ) return true;

	mDynamicCastGet(const RegionFaultBoundary*,fltbd,bd)
	if ( fltbd && fltbd->key()==mid ) return true;
    }

    return false;
}


bool Region3D::init( TaskRunner* tr )
{
    for ( int idx=0; idx<boundaries_.size(); idx++ )
    {
	RegionBoundary* bd = boundaries_[idx];
	const bool res = bd->init( tr );
	if ( !res ) return false;
    }

    return true;
}


bool Region3D::isInside( const TrcKey& tk, float z, bool ) const
{
    for ( int idx=0; idx<boundaries_.size(); idx++ )
    {
	const RegionBoundary* bd = boundaries_[idx];
	if ( !bd->onRightSide(tk,z) ) return false;
    }

    return true;
}


void Region3D::fillPar( IOPar& par ) const
{
    const int nrbd = boundaries_.size();
    par.set( "Nr Boundaries", nrbd );
    for ( int idx=0; idx<nrbd; idx ++ )
    {
	IOPar bdpar;
	const RegionBoundary* bd = boundaries_[idx];
	bd->fillPar( bdpar );
	par.mergeComp( bdpar, IOPar::compKey("Boundary",idx) );
    }
}


static RegionBoundary* createBoundary( const char* tp )
{
    const StringView type( tp );
    if ( type==sKey::Inline() ) return new RegionInlBoundary;
    if ( type==sKey::Crossline() ) return new RegionCrlBoundary;
    if ( type==sKey::ZSlice() ) return new RegionZBoundary;

    const MultiID udf = MultiID::udf();
    if ( type==sKey::Horizon() ) return new RegionHor3DBoundary(udf);
    if ( type==sKey::Fault() ) return new RegionFaultBoundary(udf);
    if ( type==sKey::Polygon() ) return new RegionPolygonBoundary(udf);
    return 0;
}


bool Region3D::usePar( const IOPar& par )
{
    int nrbd = 0;
    par.get( "Nr Boundaries", nrbd );
    for ( int idx=0; idx<nrbd; idx++ )
    {
	PtrMan<IOPar> bdpar = par.subselect( IOPar::compKey("Boundary",idx) );
	if ( !bdpar ) continue;

	BufferString type; bdpar->get( sKey::Type(), type );
	RegionBoundary* bd = !type.isEmpty() ? createBoundary( type ) : 0;
	if ( !bd ) continue;

	if ( !bd->usePar(*bdpar) )
	{
	    delete bd;
	    continue;
	}

	addBoundary( bd );
    }

    return true;
}

} // namespace EM
