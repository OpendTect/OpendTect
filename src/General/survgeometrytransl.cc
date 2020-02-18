/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Salil Agarwal
 * DATE     : Dec 2012
-*/


#include "survgeometrytransl.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "keystrs.h"
#include "survgeom2d.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "od_iostream.h"
#include "uistrings.h"

mUseType( Pos, GeomID );
mUseType( Survey, Geometry2D );

static const char* sKeyFileType()	{ return "2D Geometry"; }
static const char* sKeyAvgTrcDist()	{ return "Average Trc Distance"; }
static const char* sKeyLineLength()	{ return "Line Length"; }

defineTranslatorGroup(SurvGeom2D,"Geometry");
defineTranslator(dgb,SurvGeom2D,"2D Geometry");
mDefSimpleTranslatorSelector(SurvGeom2D);
mDefSimpleTranslatorioContext(SurvGeom2D,Geom);

uiString SurvGeom2DTranslatorGroup::sTypeName(int num)
{ return uiStrings::sGeometry(num); }


GeomID SurvGeom2DTranslator::getGeomID( const IOObj& ioobj )
{
    return geomIDOf( ioobj.key() );
}


IOObj* SurvGeom2DTranslator::getIOObj( GeomID geomid )
{
    IOObjContext ioctxt( mIOObjContext(SurvGeom2D) );
    const DBKey dbky( ioctxt.getSelDirID(), DBKey::ObjID::get(geomid.getI()) );
    return dbky.getIOObj();
}


IOObj* SurvGeom2DTranslator::getEntry( const char* name, const char* trkey )
{
    IOObjContext iocontext( mIOObjContext(SurvGeom2D) );
    if ( trkey && *trkey )
	iocontext.fixTranslator( trkey );

    CtxtIOObj ctio( iocontext );
    ctio.ctxt_.setName( name );
    if ( !ctio.fillObj() )
	return 0;

    return ctio.ioobj_;
}


Geometry2D* dgbSurvGeom2DTranslator::readGeometry( const IOObj& ioobj,
						    uiString& errmsg ) const
{
    od_istream strm( ioobj.mainFileName() );
    if ( !strm.isOK() )
    {
	errmsg = uiStrings::phrCannotOpenForRead( strm.fileName() );
	return 0;
    }

    int version = 1;
    mUnusedVar float avgtrcdist = mUdf(float);
    mUnusedVar float linelength = mUdf(float);
    ascistream astrm( strm );
    const bool hasheader = astrm.hasStandardHeader();
    if ( !hasheader )
        strm.reOpen();
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
    {
	delete data;
	errmsg = uiStrings::phrCannotRead( strm.fileName() );
	return 0;
    }
    data->setLineName( ioobj.name() );

    Geometry2D* geom = new Geometry2D( data );
    geom->setGeomID( geomIDOf(ioobj.key()) );

    auto& spnrs = geom->spNrs();
    if ( version == 3 )
    {
	for ( auto& spnr : spnrs )
	    strm.getBin( spnr );
    }
    else if ( version == 2 )
    {
	for ( auto& spnr : spnrs )
	{
	    int ispnr = -1;
	    strm.getBin( ispnr );
	    spnr = (float)ispnr;
	}
    }

    geom->commitChanges();
    return geom;
}


bool dgbSurvGeom2DTranslator::writeGeometry( IOObj& ioobj,
					     const Geometry2D& geom,
					     uiString& errmsg ) const
{
    const_cast<Geometry2D&>(geom).setGeomID( geomIDOf(ioobj.key()) );

    od_ostream strm( ioobj.mainFileName() );
    ascostream astream( strm );
    astream.putHeader( sKeyFileType() );
    astream.put( sKey::Version(), 3 );
    astream.put( sKeyAvgTrcDist(), geom.averageTrcDist() );
    astream.put( sKeyLineLength(), geom.lineLength() );
    astream.newParagraph();

    const bool res = !strm.isOK() ? false
		   : geom.data().write( strm, false, true );
    if ( !res )
	{ errmsg = strm.errMsg(); return false; }

    for ( auto spnr : geom.spNrs() )
	strm.addBin( spnr );

    return strm.isOK();
}
