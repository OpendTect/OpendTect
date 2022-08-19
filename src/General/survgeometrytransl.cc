/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "survgeometrytransl.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "od_iostream.h"
#include "posinfo2d.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "uistrings.h"

static const char* sKeyFileType()	{ return "2D Geometry"; }
static const char* sKeyAvgTrcDist()	{ return "Average Trc Distance"; }
static const char* sKeyLineLength()	{ return "Line Length"; }

defineTranslatorGroup(SurvGeom2D,"Geometry")
defineTranslator(dgb,SurvGeom2D,"2D Geometry")
mDefSimpleTranslatorSelector(SurvGeom2D)
mDefSimpleTranslatorioContext(SurvGeom2D,Geom)

uiString SurvGeom2DTranslatorGroup::sTypeName(int num)
{ return uiStrings::sGeometry(num); }


Pos::GeomID SurvGeom2DTranslator::getGeomID( const IOObj& ioobj )
{
    return Pos::GeomID( ioobj.key().objectID() );
}


IOObj* SurvGeom2DTranslator::getIOObj( Pos::GeomID geomid )
{
    IOObjContext ioctxt( mIOObjContext(SurvGeom2D) );
    MultiID mid = ioctxt.getSelKey();
    mid.setObjectID( geomid.asInt() );
    return IOM().get( mid );
}


IOObj* SurvGeom2DTranslator::createEntry( const char* name, const char* trkey )
{
    IOObjContext iocontext( mIOObjContext(SurvGeom2D) );
    if ( !IOM().to(iocontext.getSelKey()) )
	return nullptr;

    if ( trkey && *trkey )
	iocontext.fixTranslator( trkey );

    CtxtIOObj ctio( iocontext );
    ctio.ctxt_.setName( name );
    if ( ctio.fillObj() == 0 )
	return nullptr;

    return ctio.ioobj_;
}


Survey::Geometry* dgbSurvGeom2DTranslator::readGeometry( const IOObj& ioobj,
							uiString& errmsg ) const
{
    od_istream strm( ioobj.fullUserExpr() );
    if ( !strm.isOK() )
	return 0;

    int version = 1;
    float avgtrcdist = mUdf(float);
    float linelength = mUdf(float);
    ascistream astrm( strm );
    const bool hasheader = astrm.hasStandardHeader();
    if ( !hasheader )
	strm.setReadPosition( 0 );
    else
    {
	if ( astrm.atEOS() )
	    astrm.next();
	if ( astrm.hasKeyword(sKey::Version()) )
	{
	    version = astrm.getIValue();
	    astrm.next();
	}
	if ( astrm.hasKeyword(sKeyAvgTrcDist()) )
	{
	    avgtrcdist = astrm.getFValue();
	    astrm.next();
	}
	if ( astrm.hasKeyword(sKeyLineLength()) )
	{
	    linelength = astrm.getFValue();
	    astrm.next();
	}
    }

    PosInfo::Line2DData* data = new PosInfo::Line2DData;
    if ( !data->read(strm,false) )
	{ delete data; return nullptr; }

    const Pos::GeomID geomid = getGeomID( ioobj );
    data->setLineName( ioobj.name() );
    Survey::Geometry2D* geom = new Survey::Geometry2D( data );
    geom->setID( geomid );
    geom->spnrs().setSize( data->size(), -1 );
    geom->setAverageTrcDist( avgtrcdist );
    geom->setLineLength( linelength );

    if ( version > 1 )
    {
	const bool spstoredasint = version == 2;
	for ( int idx=0; idx<data->size(); idx++ )
	{
	    float spnr = -1;
	    if ( spstoredasint )
	    {
		int isp = mUdf(int);
		strm.getBin( isp );
		spnr = sCast(float,isp);
	    }
	    else
		strm.getBin( spnr );

	    geom->spnrs()[idx] = spnr;
	}
    }

    geom->touch();
    return geom;
}


bool dgbSurvGeom2DTranslator::writeGeometry( IOObj& ioobj,
					     Survey::Geometry& geom,
					     uiString& errmsg ) const
{
    RefMan<Survey::Geometry2D> geom2d = geom.as2D();
    if ( !geom2d )
	return false;

    geom2d->setID( getGeomID(ioobj) );

    od_ostream strm( ioobj.fullUserExpr() );
    ascostream astream( strm );
    astream.putHeader( sKeyFileType() );
    astream.put( sKey::Version(), 3 );
    astream.put( sKeyAvgTrcDist(), geom2d->averageTrcDist() );
    astream.put( sKeyLineLength(), geom2d->lineLength() );
    astream.newParagraph();

    const bool res = !strm.isOK() ? false
		   : geom2d->data().write( strm, false, true );
    if ( !res )
    {
	errmsg = strm.errMsg();
	return false;
    }

    for ( int idx=0; idx<geom2d->spnrs().size(); idx++ )
	strm.addBin( geom2d->spnrs()[idx] );

    return strm.isOK();
}


bool dgbSurvGeom2DTranslator::implRename( const IOObj* ioobj,
					  const char* newnm ) const
{
    const bool res = Translator::implRename( ioobj, newnm );
    if ( !res )
	return false;

    const Pos::GeomID geomid = getGeomID( *ioobj );
    RefMan<Survey::Geometry> geom = Survey::GMAdmin().getGeometry( geomid );
    Survey::Geometry2D* geom2d = geom ? geom->as2D() : nullptr;
    if ( geom2d )
	geom2d->dataAdmin().setLineName( ioobj->name() );

    return true;
}
