/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kris
 Date:          2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "surv2dgeom.h"

#include "keystrs.h"
#include "multiid.h"
#include "survinfo.h"
#include "task.h"

using namespace Survey;


mImplFactory(GeometryReader,GeometryReader::factory);
mImplFactory(GeometryWriter,GeometryWriter::factory);
static GeometryManager* theinst = 0;
const TrcKey::SurvID GeometryManager::surv2did_ = 0;


GeometryManager& Survey::GMAdmin()
{
    if( !theinst )
	{ theinst = new GeometryManager(); }

    return *theinst;
}


Geometry::Geometry()
    : id_(mUdf(ID))
{
}


Geometry::~Geometry()
{
}


TrcKey::SurvID Geometry::get2DSurvID()
{
    return GeometryManager::get2DSurvID();
}


TrcKey Geometry::getTrace( const Coord& crd, float maxdist ) const
{
    float dist;
    const TrcKey tk = nearestTrace( crd,  &dist );
    return tk.isUdf() || dist>maxdist ? TrcKey::udf() : tk;
}


bool Geometry::includes( const TrcKey& tk ) const
{
    return GM().getGeomID( tk ) == getID()
	&& includes( tk.lineNr(), tk.trcNr() );
}


Coord Geometry::toCoord( const TrcKey& tk )
{
    const Geometry* geom = GM().getGeometry( GM().getGeomID(tk)  );
    return geom ? geom->toCoord( tk.pos() ) : Coord::udf();
}


bool Geometry::exists( const TrcKey& tk )
{
    const Geometry* geom = GM().getGeometry( GM().getGeomID(tk) );
    return geom && geom->includes( tk.pos() );
}


#define mGetConstGeom(varnm,geomid) \
    ConstRefMan<Geometry> varnm = GM().getGeometry( geomid );


GeometryManager::GeometryManager()
{
}


GeometryManager::~GeometryManager()
{
    deepUnRef( geometries_ );
}


TrcKey::SurvID GeometryManager::default3DSurvID() const
{
    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( !geometries_[idx]->is2D() )
	    return geometries_[idx]->getID();
    return cUndefGeomID();
}


const Geometry* GeometryManager::getGeometry( Geometry::ID geomid ) const
{
    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( geometries_[idx]->getID() == geomid )
	    return geometries_[idx];
    return 0;
}


const Geometry* GeometryManager::getGeometry( const MultiID& mid ) const
{
    if ( mid.nrKeys() == 2 )
	return getGeometry( mid.ID(1) );

    return 0;
}


Geometry::ID GeometryManager::getGeomID( const TrcKey& tk ) const
{
    Geometry::ID geomid = tk.survID();
    if ( geomid == surv2did_ )
	geomid = tk.lineNr();
    return geomid;
}


Geometry::ID GeometryManager::getGeomID( const char* lnnm ) const
{
    const FixedString reqln( lnnm );
    for ( int idx=0; idx<geometries_.size(); idx++ )
    {
	if ( reqln == geometries_[idx]->getName() )
	    return geometries_[idx]->getID();
    }

    return cUndefGeomID();
}


const char* GeometryManager::getName( Geometry::ID geomid ) const
{
    mGetConstGeom(geom,geomid);
    return geom ? geom->getName() : 0;
}


Coord GeometryManager::toCoord( const TrcKey& tk ) const
{
    const Geometry::ID geomid = getGeomID( tk );
    mGetConstGeom(geom,geomid);
    return geom ? geom->toCoord( tk.lineNr(), tk.trcNr() ) : Coord::udf();
}


void GeometryManager::addGeometry( Survey::Geometry& geom )
{
    geom.ref();
    geometries_ += &geom;
}


bool GeometryManager::fetchFrom2DGeom()
{
    PtrMan<GeometryWriter> geomwriter = GeometryWriter::factory()
				       .create(sKey::TwoD());
    const bool makenewlinenames = hasDuplicateLineNames();
    BufferStringSet lsnames;
    S2DPOS().getLineSets( lsnames );
    for ( int lsidx=0; lsidx<lsnames.size(); lsidx++ )
    {
	BufferStringSet lnames;
	S2DPOS().getLines( lnames, lsnames.get(lsidx).buf() );
	S2DPOS().setCurLineSet( lsnames.get(lsidx).buf() );
	for ( int lidx=0; lidx<lnames.size(); lidx++ )
	{
	    PosInfo::Line2DData* data = new PosInfo::Line2DData;
	    data->setLineName( lnames.get(lidx) );
	    if ( !S2DPOS().getGeometry( *data ) )
	    {
		delete data;
		continue;
	    }

	    if ( makenewlinenames )
	    {
		BufferString newlnm = lsnames.get( lsidx );
		newlnm.add( "-" );
		newlnm.add( lnames.get(lidx) );
		data->setLineName( newlnm );
	    }

	    RefMan<Geometry2D> geom2d = new Geometry2D( data );
	    geomwriter->write( *geom2d );
	}
    }

    return true;
}


bool GeometryManager::hasDuplicateLineNames()
{
    BufferStringSet lsnames;
    S2DPOS().getLineSets( lsnames );
    BufferStringSet linenames;
    for ( int lsidx=0; lsidx<lsnames.size(); lsidx++ )
    {
	BufferStringSet lnames;
	S2DPOS().getLines( lnames, lsnames.get(lsidx).buf() );
	for ( int totalidx=0; totalidx<linenames.size(); totalidx++ )
	{
	    for ( int lidx=0; lidx<lnames.size(); lidx++ )
	    {
		if ( lnames.get(lidx) == linenames.get(totalidx) )
		    return true;
	    }
	}
	
	linenames.add( lnames, false );
    }
	
    return false;
}


bool GeometryManager::write( Geometry& geom )
{
    if ( geom.is2D() )
    {
	PtrMan<GeometryWriter> geomwriter =GeometryWriter::factory()
						    .create( sKey::TwoD() );
	geom.ref();
	if ( !geomwriter->write(geom) )
	{
	    geom.unRef();
	    return false;
	}

	addGeometry( geom );
	geom.unRef();
	return true;
    }
    else
	return false;
}


IOObj* GeometryManager::createEntry( const char* name, const bool is2d )
{
    if ( is2d )
    {
	PtrMan<GeometryWriter> geomwriter = GeometryWriter::factory()
				    .create( sKey::TwoD() );

	return geomwriter->createEntry( name );
    }
    else
	return 0;
}


void GeometryManager::removeGeometry( Geometry::ID geomid )
{
    const int index = indexOf( geomid );
    if ( geometries_.validIdx(index) )
    {
	Geometry* geom = geometries_.removeSingle( index );
	geom->unRef();
    }

    return;
    
}


int GeometryManager::indexOf( Geometry::ID geomid ) const
{
    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( geometries_[idx]->getID() == geomid )
	    return idx;

    return -1;
}

bool GeometryManager::fillGeometries( TaskRunner* tr )
{
    deepUnRef( geometries_ );
    PtrMan<GeometryReader> geomreader = GeometryReader::factory()
				        .create(sKey::TwoD());
    return geomreader->read( geometries_, tr );
}


BinID InlCrlSystem::transform( const Coord& c ) const
{
    StepInterval<Pos::LineID> inlrg; StepInterval<Pos::TraceID> crlrg;
    cs_.hrg.get( inlrg, crlrg );
    return b2c_.transformBack( c, &inlrg, &crlrg );
}


Coord InlCrlSystem::transform( const BinID& b ) const
{ return b2c_.transform(b); }


Coord3 InlCrlSystem::oneStepTranslation( const Coord3& planenormal ) const
{
    Coord3 translation( 0, 0, 0 );
    
    if ( fabs(planenormal.z) > 0.5 )
    {
	translation.z = SI().zStep();
    }
    else
    {
	Coord norm2d = planenormal;
	norm2d.normalize();
	
	if ( fabs(norm2d.dot(SI().binID2Coord().rowDir())) > 0.5 )
	    translation.x = inlDistance();
	else
	    translation.y = crlDistance();
    }
    
    return translation;
}


float InlCrlSystem::inlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c10 = transform( BinID(1,0) );
    return (float) c00.distTo(c10);
}


float InlCrlSystem::crlDistance() const
{
    const Coord c00 = transform( BinID(0,0) );
    const Coord c01 = transform( BinID(0,1) );
    return (float) c00.distTo(c01);
}
