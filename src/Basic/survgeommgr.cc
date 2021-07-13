/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kris / Bert
 Date:          2013 / Nov 2018
________________________________________________________________________

-*/

#include "survgeommgr.h"
#include "survgeom2d.h"
#include "survgeom3d.h"
#include "dbkey.h"

#include "envvars.h"
#include "genc.h"
#include "keystrs.h"
#include "survinfo.h"
#include "task.h"
#include "uistrings.h"

mUseType( Survey,	Geometry2D );
mUseType( Survey,	Geometry3D );
mUseType( Pos,		GeomID );
mUseType( SurvGM,	idx_type );
mUseType( SurvGM,	size_type );

BufferString Survey::GeometryManager::factorykey_;
mImplClassFactory(Survey::Geometry2DReader,factory);
mImplClassFactory(Survey::Geometry2DWriter,factory);

static PtrMan<SurvGM>* thesurvgm = nullptr;

static void deleteSurvGM()
{
    if ( thesurvgm )
    {
	thesurvgm->erase();
	thesurvgm = nullptr;
    }
}

PtrMan<SurvGM>& survGMMgr_()
{
    static PtrMan<SurvGM> survgmmgr_ = nullptr;
    if ( !survgmmgr_ && !IsExiting() )
    {
	auto* newsurvgmmgr = new SurvGM;
	if ( survgmmgr_.setIfNull(newsurvgmmgr,true) )
	    NotifyExitProgram( &deleteSurvGM );
    }
    return survgmmgr_;
}

const SurvGM& Survey::GM()
{
    if ( !thesurvgm )
	thesurvgm = &survGMMgr_();
    return *(*thesurvgm).ptr();
}


GeomID getDefault2DGeomID()
{
    return Survey::GM().default2DGeomID();
}


Survey::GeometryManager::GeometryManager()
{
    if ( factorykey_.isEmpty() )
    {
	const BufferString envfactky( GetEnvVar("OD_2D_GEOMETRY_STORE") );
	factorykey_ = envfactky.isEmpty() ? sKey::TwoD().str()
					  : envfactky.str();
    }
}


Survey::GeometryManager::~GeometryManager()
{
    deepUnRef( geometries_ );
}


const Geometry3D& Survey::GeometryManager::get3DGeometry() const
{
    return Geometry::get3D();
}


const Survey::Geometry* Survey::GeometryManager::getGeometry(
						const char* nm ) const
{
    if ( IsExiting() )
	return nullptr;

    const Geometry3D& sigeom = get3DGeometry();
    if ( sigeom.hasName(nm) )
	return &sigeom;

    return get2DGeometry( nm );
}


const Survey::Geometry2D* Survey::GeometryManager::get2DGeometry(
							GeomID gid ) const
{
    if ( IsExiting() )
	return nullptr;

    Threads::Locker locker( geometrieslock_ );
    for ( const auto* geom : geometries_ )
	if ( geom->geomID() == gid )
	    return geom;

    return nullptr;
}


const Survey::Geometry2D* Survey::GeometryManager::get2DGeometry(
						const char* nm ) const
{
    if ( IsExiting() )
	return nullptr;

    Threads::Locker locker( geometrieslock_ );
    for ( const auto* geom : geometries_ )
	if ( geom->hasName(nm) )
	    return geom;

    return nullptr;
}


GeomID Survey::GeometryManager::default2DGeomID() const
{
    //TODO proper impl
    return geometries_.isEmpty() ? GeomID(0) : geometries_.first()->geomID();
}


size_type Survey::GeometryManager::nr2DGeometries() const
{
    Threads::Locker locker( geometrieslock_ );
    return geometries_.size();
}


idx_type Survey::GeometryManager::indexOf( GeomID geomid ) const
{
    Threads::Locker locker( geometrieslock_ );
    return gtIndexOf( geomid );
}


const Geometry2D* Survey::GeometryManager::get2DGeometryByIdx(
						idx_type idx ) const
{
    Threads::Locker locker( geometrieslock_ );
    return geometries_.validIdx(idx) ? geometries_.get( idx ) : 0;
}


int Survey::GeometryManager::gtIndexOf( GeomID geomid ) const
{
    for ( int idx=0; idx<geometries_.size(); idx++ )
	if ( geometries_[idx]->geomID() == geomid )
	    return idx;

    return -1;
}


Survey::Geometry* Survey::GeometryManager::gtGeometry( GeomID geomid ) const
{
    if ( IsExiting() )
	return nullptr;

    if ( geomid == GeomID::get3D() )
	return const_cast<Geometry3D*>( &Geometry::get3D() );
    else if ( !geomid.isValid() )
	return nullptr;

    Threads::Locker locker( geometrieslock_ );
    const int idx = gtIndexOf( geomid );
    if ( idx >= 0 )
	return const_cast<Geometry2D*>( geometries_[idx] );

    pErrMsg( "Geometry ID not present, most probably programmer error" );
    return nullptr;
}


GeomID Survey::GeometryManager::getGeomID( const char* lnnm ) const
{
    const auto* geom = get2DGeometry( lnnm );
    return geom ? geom->geomID() : GeomID();
}


const char* Survey::GeometryManager::getName( GeomID geomid ) const
{
    const auto* geom = getGeometry( geomid );
    return geom ? geom->name().str() : nullptr;
}


bool Survey::GeometryManager::fillGeometries( const TaskRunnerProvider& trprov )
{
    Threads::Locker locker( geometrieslock_ );
    deepUnRef( geometries_ );
    PtrMan<Geometry2DReader> geomreader = Geometry2DReader::factory()
					.create(factorykey_);
    return geomreader && geomreader->read( geometries_, trprov );
}


void Survey::GeometryManager::list2D( GeomIDSet& geomids,
					BufferStringSet* names ) const
{
    geomids.erase();
    if ( names )
	names->erase();
    Threads::Locker locker( geometrieslock_ );
    for ( const auto* geom : geometries_ )
    {
	geomids += geom->geomID();
	if ( names )
	    names->add( geom->name() );
    }
}


Pos::GeomID Survey::GeometryManager::findRelated( const Geometry& ref,
					   Geometry::RelationType& reltype,
					   bool usezrg ) const
{
    if ( ref.is3D() )
    {
	const auto& sigeom3d = Geometry::get3D();
	reltype = ref.as3D()->compare( sigeom3d, usezrg );
	return sigeom3d.geomID();
    }

    reltype = Geometry::UnRelated;
    if ( geometries_.isEmpty() )
	return GeomID();

    GeomID geomid;
    const Geometry2D& ref2d = *ref.as2D();
    int identicalidx=-1, supersetidx=-1, subsetidx=-1, relatedidx=-1;
    for ( int idx=0; identicalidx<0 && idx<geometries_.size(); idx++ )
    {
	const auto& geom = *geometries_[idx];
	geomid = geom.geomID();
	switch( geom.compare(ref2d,usezrg) )
	{
#	    define mCaseTypeSetToIdx(specidx,reltyp) \
	    case Geometry::reltyp: specidx = idx; break
	    mCaseTypeSetToIdx( identicalidx, Identical );
	    mCaseTypeSetToIdx( supersetidx, SuperSet );
	    mCaseTypeSetToIdx( subsetidx, SubSet );
	    mCaseTypeSetToIdx( relatedidx, Related );
	    default: break;
	}
    }

#   define mRetGeomIDIfIdxOK(idx,reltyp) \
    if ( idx >= 0 ) \
	{ reltype = Geometry::reltyp; return geometries_[idx]->geomID(); }
    mRetGeomIDIfIdxOK( identicalidx, Identical );
    mRetGeomIDIfIdxOK( supersetidx, SuperSet );
    mRetGeomIDIfIdxOK( subsetidx, SubSet );
    mRetGeomIDIfIdxOK( relatedidx, Related );

    return geomid;
}


bool Survey::GeometryManager::save( const Geometry2D& geom, uiString& errmsg,
				    Geometry2DWriter* wrr ) const
{
    PtrMan<Geometry2DWriter> destroyer;
    if ( !wrr )
    {
	wrr = Geometry2DWriter::factory().create( factorykey_ );
	destroyer = wrr;
	if ( !wrr )
	{
	    errmsg = mINTERNAL("Geometry2DWriter::factory problem");
	    return false;
	}
    }

    return wrr->write( geom, errmsg );
}


bool Survey::GeometryManager::addEntry( Geometry2D* geom, GeomID& geomid,
					uiString& errmsg )
{
    geomid = GeomID();
    if ( !geom )
	{ GeomID(); errmsg = mINTERNAL( "Null geometry" ); return false; }

    const auto existgeomid = getGeomID( geom->name() );
    if ( existgeomid.isValid() )
	{ geomid = existgeomid; return true; }

    PtrMan<Geometry2DWriter> wrr =
	Geometry2DWriter::factory().create( factorykey_ );
    if ( !wrr )
    {
	errmsg = mINTERNAL( BufferString("No writer for <",factorykey_,">") );
	return false;
    }

    auto newgeomid = wrr->getGeomIDFor( geom->name() );
    if ( !newgeomid.isValid() )
    {
	errmsg = uiStrings::phrCannotCreateDBEntryFor( uiStrings::sGeometry() );
	return false;
    }

    geom->setGeomID( newgeomid );
    geomid = GeomID();
    if ( !save(*geom,errmsg,wrr) )
	return false;

    geomid = newgeomid;
    geom->ref();
    Threads::Locker locker( geometrieslock_ );
    geometries_.add( geom );
    return true;
}


bool Survey::GeometryManager::removeGeometry( GeomID geomid )
{
    Threads::Locker locker( geometrieslock_ );
    const int idx = gtIndexOf( geomid );
    if ( idx < 0 )
	return false;

    geometries_.removeSingle(idx)->unRef();
    return true;
}


bool Survey::GeometryManager::updateGeometries(
				const TaskRunnerProvider& trprov ) const
{
    PtrMan<Geometry2DReader> geomreader = Geometry2DReader::factory()
					.create(factorykey_);
    if ( !geomreader )
	return false;

    Threads::Locker locker( geometrieslock_ );
    return geomreader->updateGeometries(
	    const_cast<GeometryManager*>(this)->geometries_, trprov );
}
