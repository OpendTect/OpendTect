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
#include "executor.h"

using namespace Survey;


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

	od_int32 nextStep()
	{
		StreamData isd;
		const IOObj* ioobj = objs_[nrdone_];
		const BufferString srcfile = ioobj->fullUserExpr();
		isd = StreamProvider( srcfile ).makeIStream();
		if( !isd.usable() )
		{
			nrdone_++;
			return nrdone_<totalNr() ? MoreToDo() : Finished();
		}

		if ( ioobj->translator() == dgb2DSurvGeomTranslator::translKey())
		{
			Geometry2D* geom = new Geometry2D();
			if ( !geom->data().read(*(isd.istrm),false) )
			{
				nrdone_++;
				return nrdone_<totalNr() ? MoreToDo() : Finished();
			}

			isd.close();
			geom->setGeomID( ioobj->key().ID(1) );
			geom->data().setLineName( ioobj->name() );
			geometries_ += geom;
			nrdone_++;
			return nrdone_<totalNr() ? MoreToDo() : Finished();
		}

		return Finished();
	}

	const ObjectSet<IOObj>&		objs_;
	ObjectSet<Geometry>&		geometries_;

	od_int32					nrdone_;
};


void GeometryWriter2D::initClass()
{
	GeometryWriter::factory().addCreator( create2DWriter, sKey::TwoD() );
}


bool GeometryWriter2D::write( Geometry* geom )
{
	Geometry2D* geom2d;
	mDynamicCast( Geometry2D*, geom2d, geom );
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
	GeometryWriter::factory().addCreator( create3DWriter, sKey::ThreeD() );
}


bool GeometryReader2D::read( ObjectSet<Geometry>& geometries )
{
	IOObjContext iocontext = mIOObjContext(SurvGeom);
	MultiID mid = iocontext.getSelKey();
	IOM().to(mid,true);
	const ObjectSet<IOObj> objs = IOM().dirPtr()->getObjs();
	GeomFileReader gfr( objs, geometries );
	if ( !gfr.execute() )
		return false;

	return true;
}


void GeometryReader2D::initClass()
{
	GeometryReader::factory().addCreator( create2DReader, sKey::TwoD() );
}


void GeometryReader3D::initClass()
{
	GeometryReader::factory().addCreator( create3DReader, sKey::ThreeD() );
}

