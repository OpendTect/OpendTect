/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geojsonwriter.h"
#include "survinfo.h"
#include "latlong.h"
#include "color.h"
#include "coordsystem.h"
#include "uistrings.h"


#define mErrRet(s) { errmsg_ = s; return false; }


GeoJSONWriter::GeoJSONWriter()
{}


GeoJSONWriter::~GeoJSONWriter()
{
    errmsg_.setEmpty();
    close();
}

bool GeoJSONWriter::open( const char* fnm )
{
    errmsg_.setEmpty();

    if ( !fnm || !*fnm )
	mErrRet( tr("No file name provided"))

    if ( !strm_ || !strm_->isOK() )
	mErrRet( uiStrings::phrCannotOpenForWrite( fnm ) )

    if ( !strm().isOK() )
    {
	uiString emsg( tr("Error during write of GeoJSON header info") );
	strm().addErrMsgTo( emsg );
	mErrRet(emsg)
    }

    return true;
}


void GeoJSONWriter::setStream( const BufferString& fnm )
{
    strm_ = new od_ostream( fnm );
    geojsontree_ = new OD::GeoJsonTree();
    open( fnm );
}

bool GeoJSONWriter::close()
{
    return GISWriter::close();
}



bool GeoJSONWriter::writePoint( const LatLong& ll, const char* nm )
{
    pErrMsg("Not implemented yet");
    return false;
}


bool GeoJSONWriter::writePoint( const Coord& coord, const char* nm )
{
    TypeSet<Coord> crds;
    crds += coord;
    BufferStringSet nms;
    nms.add( nm );
    return writeGeometry( "Point", crds, nms );
}


bool GeoJSONWriter::writePolygon( const RefObjectSet<const Pick::Set>& picks )
{
    return writeGeometry( "Polygon", picks );
}


bool GeoJSONWriter::writePoint( const RefObjectSet<const Pick::Set>& picks )
{
    return writeGeometry( "MultiPoint", picks );
}


bool GeoJSONWriter::writeLine( const TypeSet<Coord>& crdset, const char* nm )
{
    BufferStringSet nms;
    nms.add( nm );
    return writeGeometry( "LineString", crdset, nms );
}


bool GeoJSONWriter::writeLine( const RefObjectSet<const Pick::Set>& picks )
{
    return writeGeometry( "LineString", picks );
}


bool GeoJSONWriter::writePolygon( const TypeSet<Coord>& crdset, const char* nm )
{
    BufferStringSet nms;
    nms.add( nm );
    return writeGeometry( "Polygon", crdset, nms );
}


bool GeoJSONWriter::writePolygon( const TypeSet<Coord3>& crdset, const char* nm)
{
    BufferStringSet nms;
    nms.add( nm );
    return writeGeometry( "Polygon", crdset, nms );
}


bool GeoJSONWriter::writePoints( const TypeSet<Coord>& crds,
						    const BufferStringSet& nms )
{
    return writeGeometry( "Point", crds, nms );
}

//NEED TO SUPPORT MULTIPLE PROPERTIES
// EACH OBJECT WILL HAVE DIFFERENT PROPERTY
// NEED TO WORK ON IT

#define mSyntaxEOL( str ) \
    str.add( ", " ).addNewLine();


bool GeoJSONWriter::writeGeometry( BufferString geomtyp,
				   const TypeSet<Coord>& crdset,
				   const BufferStringSet& nms )
{
    if ( !isOK() )
	return false;

    OD::GeoJsonTree::ValueSet* valueset = geojsontree_->createJSON( geomtyp,
					crdset, nms, coordsys_, properties_ );
    BufferString str;
    valueset->dumpJSon( str );
    strm() << str;

    return true;
}


bool GeoJSONWriter::writeGeometry( BufferString geomtyp,
				   const TypeSet<Coord3>& crdset,
				   const BufferStringSet& nms )
{
    if ( !isOK() )
	return false;

    OD::GeoJsonTree::ValueSet* valueset = geojsontree_->createJSON( geomtyp,
					crdset, nms, coordsys_, properties_ );
    BufferString str;
    valueset->dumpJSon( str );
    strm() << str;

    return true;
}


bool GeoJSONWriter::writeGeometry( BufferString geomtyp,
				   const RefObjectSet<const Pick::Set>& picks )
{
    if ( !isOK() )
	return false;

    OD::GeoJsonTree::ValueSet* valueset = geojsontree_->createJSON( geomtyp,
							picks, coordsys_ );
    BufferString str;
    valueset->dumpJSon( str );
    strm() << str;

    return true;
}
