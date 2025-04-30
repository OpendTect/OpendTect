/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometryio.h"

#include "bendpointfinder.h"
#include "ctxtioobj.h"
#include "executor.h"
#include "genc.h"
#include "iodir.h"
#include "ioman.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survgeometrytransl.h"
#include "survinfo.h"

namespace Survey
{

#define mReturn return ++nrdone_ < totalNr() ? MoreToDo() : Finished();

class GeomFileReader : public Executor
{
public:

    GeomFileReader( const ObjectSet<IOObj>& objs,
		    RefObjectSet<Geometry>& geometries, bool updateonly )
	: Executor( "Loading Files" )
	, objs_(objs)
	, geometries_(geometries)
	, updateonly_(updateonly)
    {
	mAttachCB( SurveyInfo::rootDirChanged(),
		   GeomFileReader::rootDirChangedCB );
    }

    ~GeomFileReader()
    {
	detachAllNotifiers();
    }


    od_int64 nrDone() const override
    { return nrdone_; }


    od_int64 totalNr() const override
    { return objs_.size(); }

private:

    bool shouldContinue() override
    {
	return continue_ && !IsExiting();
    }

    void rootDirChangedCB( CallBacker* )
    {
	continue_ = false;
    }

    int indexOf( const Pos::GeomID& geomid ) const
    {
	for ( int idx=0; idx<geometries_.size(); idx++ )
	    if ( geometries_[idx]->getID() == geomid )
		return idx;
	return -1;
    }

    bool doPrepare( od_ostream* strm ) override
    {
	if ( !shouldContinue() )
	    return false;

	return Executor::doPrepare( strm );
    }

    int nextStep() override
    {
	if ( totalNr() == 0 )
	    return Finished();

	const IOObj* ioobj = objs_[int(nrdone_)];
	const Pos::GeomID geomid = SurvGeom2DTranslator::getGeomID( *ioobj );
	bool doupdate = false;
	const int geomidx = indexOf( geomid );
	if ( updateonly_ && geomidx!=-1 )
	{
	    mDynamicCastGet(Geometry2D*,geom2d,geometries_[geomidx])
	    if ( geom2d && geom2d->data().isEmpty() )
		doupdate = true;
	    else if ( geom2d && geom2d->data().getBendPoints().isEmpty() )
	    {
		calcBendPoints( geom2d->dataAdmin() );
		mReturn
	    }
	    else
		mReturn
	}

	PtrMan<Translator> transl = ioobj->createTranslator();
	mDynamicCastGet(SurvGeom2DTranslator*,geomtransl,transl.ptr());
	if ( !geomtransl || !geomtransl->isUsable() )
	    mReturn

	uiString errmsg;
	RefMan<Geometry> geom = geomtransl->readGeometry( *ioobj, errmsg );
	mDynamicCastGet(Geometry2D*,geom2d,geom.ptr())
	if ( geom2d )
	{
	    calcBendPoints( geom2d->dataAdmin() );
	    if ( doupdate )
		geometries_.replace( geomidx, geom.ptr() );
	    else
		geometries_.add( geom.ptr() );;
	}

	mReturn
    }

    bool doFinish( bool success, od_ostream* strm ) override
    {
	return success && shouldContinue() ? Executor::doFinish( success, strm )
					   : false;
    }

    bool calcBendPoints( PosInfo::Line2DData& l2d ) const
    {
	BendPointFinder2DGeom bpf( l2d.positions(), 10.0 );
	if ( !bpf.execute() || bpf.bendPoints().isEmpty() )
	    return false;

	l2d.setBendPoints( bpf.bendPoints() );
	return true;
    }

    bool isLoaded( const Pos::GeomID& geomid ) const
    {
	for ( const auto* geom : geometries_ )
	    if ( geom->getID() == geomid )
		return true;

	return false;
    }

    const ObjectSet<IOObj>&	objs_;
    RefObjectSet<Geometry>&	geometries_;
    od_int64			nrdone_		= 0;
    bool			updateonly_;
    bool			continue_	= true;

};


// GeometryWriter2D
GeometryWriter2D::GeometryWriter2D()
    : GeometryWriter()
{}


GeometryWriter2D::~GeometryWriter2D()
{}


void GeometryWriter2D::initClass()
{ GeometryWriter::factory().addCreator( create2DWriter, sKey::TwoD() ); }


bool GeometryWriter2D::write( Geometry& geom, uiString& errmsg,
			      const char* createfromstr ) const
{
    RefMan< Geometry2D > geom2d;
    mDynamicCast(Geometry2D*,geom2d,&geom);
    if ( !geom2d )
	return false;

    PtrMan< IOObj > ioobj = createEntry( geom2d->data().lineName().buf() );
    if ( !ioobj || ioobj->key().nrIDs() != 2)
	return false;

    PtrMan<Translator> transl = ioobj->createTranslator();
    mDynamicCastGet(SurvGeom2DTranslator*,geomtransl,transl.ptr());
    if ( !geomtransl )
	return false;

    const StringView crfromstr( createfromstr );
    if ( !crfromstr.isEmpty() )
    {
	ioobj->pars().set( sKey::CrFrom(), crfromstr );
	IOM().commitChanges( *ioobj );
    }

    return geomtransl->writeGeometry( *ioobj, geom, errmsg );
}


Pos::GeomID GeometryWriter2D::createNewGeomID( const char* name ) const
{
    PtrMan<IOObj> geomobj = createEntry( name );
    if ( !geomobj )
	return Pos::GeomID::udf();

    return SurvGeom2DTranslator::getGeomID( *geomobj );
}


IOObj* GeometryWriter2D::createEntry( const char* name ) const
{
    return SurvGeom2DTranslator::createEntry( name,
					dgbSurvGeom2DTranslator::translKey() );
}


// GeometryWriter3D
GeometryWriter3D::GeometryWriter3D()
    : GeometryWriter()
{}


GeometryWriter3D::~GeometryWriter3D()
{}


void GeometryWriter3D::initClass()
{
    GeometryWriter::factory().addCreator( create3DWriter, sKey::ThreeD() );
}


// GeometryReader2D
GeometryReader2D::GeometryReader2D()
    : GeometryReader()
{}


GeometryReader2D::~GeometryReader2D()
{}


bool GeometryReader2D::read( RefObjectSet<Geometry>& geometries,
			const ObjectSet<IOObj>& objs, TaskRunner* tr ) const
{
    GeomFileReader gfr( objs, geometries, false );
    return TaskRunner::execute( tr, gfr );
}


bool GeometryReader2D::read( RefObjectSet<Geometry>& geometries,
			     TaskRunner* tr ) const
{
    const IOObjContext& iocontext = mIOObjContext(SurvGeom2D);
    const IODir iodir( iocontext.getSelKey() );
    if ( iodir.isBad() )
	return false;

    const ObjectSet<IOObj>& objs = iodir.getObjs();
    return read( geometries, objs, tr );
}


bool GeometryReader2D::updateGeometries( RefObjectSet<Geometry>& geometries,
					 TaskRunner* tskr ) const
{
    const IOObjContext& iocontext = mIOObjContext(SurvGeom2D);
    const IODir iodir( iocontext.getSelKey() );
    if ( iodir.isBad() )
	return false;

    const ObjectSet<IOObj>& objs = iodir.getObjs();
    GeomFileReader gfr( objs, geometries, true );
    return TaskRunner::execute( tskr, gfr );
}


void GeometryReader2D::initClass()
{
    GeometryReader::factory().addCreator( create2DReader, sKey::TwoD() );
}


// GeometryReader3D
GeometryReader3D::GeometryReader3D()
    : GeometryReader()
{}


GeometryReader3D::~GeometryReader3D()
{}


void GeometryReader3D::initClass()
{
    GeometryReader::factory().addCreator( create3DReader, sKey::ThreeD() );
}

} // namespace Survey
