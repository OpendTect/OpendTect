/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : March 2021
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

    /*Do we need some header for file ??
    */
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


#define mIsMulti( object ) \
    const int sz = object.size(); \
    if ( !sz ) return; \
    const bool ismulti = sz > 1;

bool GeoJSONWriter::writePoint( const Coord& coord, const char* nm )
{
    TypeSet<Coord> crds;  crds += coord;
    BufferStringSet nms;
    nms.add( nm );
    return writeGeometry( "Point", crds, nms );
}


bool GeoJSONWriter::writePolygon( const pickset& picks )
{
    return writeGeometry( "Polygon", picks );
}


bool GeoJSONWriter::writePoint( const pickset& picks )
{
    return writeGeometry( "MultiPoint", picks );
}


bool GeoJSONWriter::writeLine( const coord2dset& crdset, const char* nm )
{
    BufferStringSet nms;
    nms.add( nm );
    return writeGeometry( "LineString", crdset, nms );
}


bool GeoJSONWriter::writeLine( const pickset& picks )
{
    return writeGeometry( "LineString", picks );
}


bool GeoJSONWriter::writePolygon( const coord2dset& crdset, const char* nm )
{
    BufferStringSet nms;
    nms.add( nm );
    return writeGeometry( "Polygon", crdset, nms );
}


bool GeoJSONWriter::writePolygon( const coord3dset& crdset, const char* nm )
{
    BufferStringSet nms;
    nms.add( nm );
    return writeGeometry( "Polygon", crdset, nms );
}


bool GeoJSONWriter::writePoints( const coord2dset& crds,
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
			const coord2dset& crdset, const BufferStringSet& nms )
{
    if ( !isOK() )
	return false;

    OD::GeoJsonTree::ValueSet* valueset = geojsontree_->createJSON( geomtyp,
						crdset, nms, coordsys_ );
    BufferString str;
    valueset->dumpJSon( str );
    strm() << str;

    return true;
}


bool GeoJSONWriter::writeGeometry( BufferString geomtyp,
			const coord3dset& crdset, const BufferStringSet& nms )
{
    if ( !isOK() )
	return false;

    OD::GeoJsonTree::ValueSet* valueset = geojsontree_->createJSON( geomtyp,
						crdset, nms, coordsys_ );
    BufferString str;
    valueset->dumpJSon( str );
    strm() << str;

    return true;
}


bool GeoJSONWriter::writeGeometry( BufferString geomtyp,
					const pickset& picks )
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
