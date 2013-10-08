/*+
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* AUTHOR   : Salil Agarwal
* DATE     : Dec 2012
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "geometryio.h"

#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "surv2dgeom.h"
#include "survgeometrytransl.h"
#include "survinfo.h"
#include "executor.h"
#include "keystrs.h"
#include "od_iostream.h"

using namespace Survey;

#define mReturn return ++nrdone_ < totalNr() ? MoreToDo() : Finished();

class GeomFileReader : public Executor
{
public:

    GeomFileReader( const ObjectSet<IOObj>& objs,
	    	    ObjectSet<Geometry>& geometries )
	: Executor( "Loading Files" )
	, objs_(objs)
	, geometries_(geometries)
	, nrdone_(0)
    {}

	    
    od_int64 nrDone() const
    { return nrdone_; }


    od_int64 totalNr() const
    { return objs_.size(); }

protected:

    int nextStep()
    {
	const IOObj* ioobj = objs_[mCast(int,nrdone_)];
	if ( ioobj->translator() != dgb2DSurvGeomTranslator::translKey() || 
	     ioobj->key().nrKeys() != 2 )
	    mReturn

	od_istream strm( ioobj->fullUserExpr() );
	if ( !strm.isOK() )
	    mReturn
	    
	PosInfo::Line2DData* data = new PosInfo::Line2DData;
	if ( !data->read(strm,false) )
	    { delete data; mReturn }

	data->setLineName( ioobj->name() );
	Geometry2D* geom = new Geometry2D( data );
	geom->setID( ioobj->key().ID(1) );
	geom->ref();
	geometries_ += geom;

	mReturn
    }

    const ObjectSet<IOObj>&	objs_;
    ObjectSet<Geometry>&	geometries_;
    od_int64		    	nrdone_;

};


void GeometryWriter2D::initClass()
{ GeometryWriter::factory().addCreator( create2DWriter, sKey::TwoD() ); }


bool GeometryWriter2D::write( Geometry& geom ) const
{
    RefMan< Geometry2D > geom2d;
    mDynamicCast( Geometry2D*, geom2d, &geom );
    if ( !geom2d )
	return false;

    PtrMan< IOObj > ioobj = createEntry( geom2d->data().lineName().buf() );
    if ( !ioobj || ioobj->key().nrKeys() != 2)
	return false;

    geom2d->setID( ioobj->key().ID(1) );
    od_ostream strm( ioobj->fullUserExpr() );
    const bool res = !strm.isOK() ? false
		   : geom2d->data().write( strm, false, true );
    return res;
}


IOObj* GeometryWriter2D::createEntry( const char* name ) const
{
    const IOObjContext& iocontext = mIOObjContext(SurvGeom);
    if ( !IOM().to(iocontext.getSelKey()) )
	return 0;

    CtxtIOObj ctio( iocontext );
    ctio.ctxt.setName( name );
    if ( ctio.fillObj() == 0 )
	return 0;

    return ctio.ioobj;
}


void GeometryWriter3D::initClass()
{
    GeometryWriter::factory().addCreator( create3DWriter, sKey::ThreeD() );
}


bool GeometryReader2D::read( ObjectSet<Geometry>& geometries, 
			     TaskRunner* tr ) const
{
    const IOObjContext& iocontext = mIOObjContext(SurvGeom);
    const MultiID mid = iocontext.getSelKey();
    if ( !IOM().to(mid,true) )
	return false;
	
    const ObjectSet<IOObj> objs = IOM().dirPtr()->getObjs();
    GeomFileReader gfr( objs, geometries );
    return TaskRunner::execute( tr, gfr );
}


void GeometryReader2D::initClass()
{
    GeometryReader::factory().addCreator( create2DReader, sKey::TwoD() );
}


void GeometryReader3D::initClass()
{
    GeometryReader::factory().addCreator( create3DReader, sKey::ThreeD() );
}

