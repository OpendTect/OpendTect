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
#include "iostrm.h"
#include "iosubdir.h"
#include "surv2dgeom.h"
#include "survgeometrytransl.h"
#include "strmprov.h"
#include "survinfo.h"
#include "task.h"

using namespace Survey;


class GeomFileReader : public ::ParallelTask
{
public:
GeomFileReader( const ObjectSet<IOObj>& objs, ObjectSet<Geometry>& geometries )
    : objs_(objs)
    , geometries_(geometries)
{}

    
od_int64 nrIterations() const 
{ return objs_.size(); }

    
bool doWork( od_int64 start, od_int64 stop, int )
{
    StreamData isd;
    for ( int idx=(int) start; idx<=stop; idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	const BufferString srcfile = ioobj->fullUserExpr();
        isd = StreamProvider( srcfile ).makeIStream();
        if( !isd.usable() )
	    return false;

	if ( ioobj->translator()==dgb2DSurvGeomTranslator::translKey())
	{
	    Geometry2D* geom = (Geometry2D*) geometries_[idx];
	    if ( !geom->data().read(*(isd.istrm),false) )
		return false;

	    isd.close();
	    geom->setGeomID( ioobj->key().ID(1) );
	}
	else
	{
	    //TODO 3D to be implemented
	    isd.close();
	}
    }

    return true;
}

protected:

    const ObjectSet<IOObj>&	objs_;
    ObjectSet<Geometry>&	geometries_;
};


void GeometryWriter2D::initClass()
{
    GeometryWriter::factory().addCreator( create2DWriter, "2D" );
}


bool GeometryWriter2D::write( Geometry* geom )
{
    Geometry2D* geom2d = (Geometry2D*)geom;
    geom2d->ref();
    IOObjContext iocontext = mIOObjContext(SurvGeom);
    iocontext.setName("Geometry");
    MultiID mid = iocontext.getSelKey();
    IOM().to( mid );
    CtxtIOObj cioob( iocontext );
    cioob.ctxt.setName( geom2d->data().lineName().buf() );
    cioob.ioobj = 0;
    cioob.fillObj();
    mid.add( cioob.ioobj->key().ID(1) );
    geom2d->setGeomID( cioob.ioobj->key().ID(1) );
    BufferString destfile = IOM().get( mid )->fullUserExpr();
    StreamData osd = StreamProvider( destfile ).makeOStream();
    
    if ( osd.usable() )
    {
	if ( !(geom2d->data().write(*(osd.ostrm),false,true)) )
	    return false;
    }

    osd.close();
    geom2d->unRef();
    return true;
}


int GeometryWriter2D::createEntry( const char* name )
{
    IOObjContext iocontext = mIOObjContext(SurvGeom);
    iocontext.setName("Geometry");
    IOM().to( iocontext.getSelKey() );
    CtxtIOObj cioob( iocontext );
    cioob.ctxt.setName( name );
    cioob.ioobj = 0;
    cioob.fillObj();
    return cioob.ioobj->key().ID(1);
}


void GeometryWriter3D::initClass()
{
    GeometryWriter::factory().addCreator( create3DWriter, "3D" );
}


bool GeometryReader2D::read( ObjectSet<Geometry>& geometries )
{
    IOObjContext iocontext = mIOObjContext(SurvGeom);
    MultiID mid = iocontext.getSelKey();
    IOM().to(mid,true);
    const ObjectSet<IOObj> objs = IOM().dirPtr()->getObjs();
    for ( int idx=0; idx<objs.size(); idx++ )
    {
	if ( objs[idx]->translator() == dgb2DSurvGeomTranslator::translKey() )
	    geometries += new Geometry2D();
	else
	{}//TODO 3D to be implemented
    }

    GeomFileReader gfr( objs, geometries );
    if ( !gfr.execute() )
	return false;

    return true;
}


void GeometryReader2D::initClass()
{
    GeometryReader::factory().addCreator( create2DReader, "2D" );
}


void GeometryReader3D::initClass()
{
    GeometryReader::factory().addCreator( create3DReader, "3D Reader" );
}
