/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Dec 2012
-*/


#include "survgeometrytransl.h"
#include "ctxtioobj.h"
#include "dbdir.h"
#include "dbman.h"
#include "survgeom2d.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "od_iostream.h"
#include "uistrings.h"


defineTranslatorGroup(SurvGeom2D,"Geometry");
defineTranslator(dgb,SurvGeom2D,"2D Geometry");
mDefSimpleTranslatorSelector(SurvGeom2D);
mDefSimpleTranslatorioContext(SurvGeom2D,Geom);

uiString SurvGeom2DTranslatorGroup::sTypeName(int num)
{ return uiStrings::sGeometry(num); }


Pos::GeomID SurvGeom2DTranslator::getGeomID( const IOObj& ioobj )
{
    return ioobj.objID().getI();
}


IOObj* SurvGeom2DTranslator::getIOObj( Pos::GeomID geomid )
{
    IOObjContext ioctxt( mIOObjContext(SurvGeom2D) );
    const DBKey dbky( ioctxt.getSelDirID(), DBKey::ObjID::get(geomid) );
    return DBM().get( dbky );
}


IOObj* SurvGeom2DTranslator::createEntry( const char* name, const char* trkey )
{
    IOObjContext iocontext( mIOObjContext(SurvGeom2D) );
    if ( trkey && *trkey )
	iocontext.fixTranslator( trkey );

    CtxtIOObj ctio( iocontext );
    ctio.ctxt_.setName( name );
    if ( ctio.fillObj() == 0 )
	return 0;

    return ctio.ioobj_;
}


Survey::Geometry* dgbSurvGeom2DTranslator::readGeometry( const IOObj& ioobj,
							uiString& errmsg ) const
{
    od_istream strm( ioobj.fullUserExpr() );
    if ( !strm.isOK() )
	return 0;

    PosInfo::Line2DData* data = new PosInfo::Line2DData;
    if ( !data->read(strm,false) )
	{ delete data; return 0; }

    const Survey::Geometry::ID geomid = ioobj.key().objID().getI();
    data->setLineName( ioobj.name() );
    Survey::Geometry2D* geom = new Survey::Geometry2D( data );
    geom->setID( geomid );
    geom->touch();
    return geom;
}


bool dgbSurvGeom2DTranslator::writeGeometry( IOObj& ioobj,
					     Survey::Geometry& geom,
					     uiString& errmsg ) const
{
    RefMan<Survey::Geometry2D> geom2d;
    mDynamicCast( Survey::Geometry2D*, geom2d, &geom );
    if ( !geom2d )
	return false;

    geom2d->setID( ioobj.key().objID().getI() );
    od_ostream strm( ioobj.fullUserExpr() );
    const bool res = !strm.isOK() ? false
		   : geom2d->data().write( strm, false, true );
    if ( !res )
	errmsg = strm.errMsg();

    return res;
}
