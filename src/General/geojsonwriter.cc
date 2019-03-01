/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : October 2018
-*/

#include "geojsonwriter.h"
#include "survinfo.h"
#include "latlong.h"
#include "color.h"
#include "coordsystem.h"
#include "uistrings.h"


#define mErrRet(s) { errmsg_ = s; return false; }

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

void GeoJSONWriter::close()
{
    strm_->close();
}


#define mIsMulti( object ) \
    const int sz = object.size(); \
    if ( !sz ) return; \
    const bool ismulti = sz > 1;

void GeoJSONWriter::writePoint( const Coord& coord, const char* nm )
{
    TypeSet<Coord> crds;  crds += coord;
    BufferStringSet nms;
    nms.add( nm );
    writeGeometry( "Point", crds, nms );
}


void GeoJSONWriter::writePolygon( const pickset& picks )
{
    writeGeometry( "Polygon", picks );
}


void GeoJSONWriter::writePoint( const pickset& picks )
{
    writeGeometry( "Point", picks );
}


void GeoJSONWriter::writeLine( const coord2dset& crdset, const char* nm )
{
    BufferStringSet nms;
    nms.add( nm );
    writeGeometry( "LineString", crdset, nms );
}


void GeoJSONWriter::writeLine( const pickset& picks )
{
    writeGeometry( "LineString", picks );
}


void GeoJSONWriter::writePolygon( const coord2dset& crdset, const char* nm )
{
    BufferStringSet nms;
    nms.add( nm );
    writeGeometry( "Polygon", crdset, nms );
}


void GeoJSONWriter::writePolygon( const coord3dset& crdset, const char* nm )
{
    BufferStringSet nms;
    nms.add( nm );
    writeGeometry( "Polygon", crdset, nms );
}


void GeoJSONWriter::writePoints( const coord2dset& crds,
						    const BufferStringSet& nms )
{
    writeGeometry( "Point", crds, nms );
}



#define mSyntaxEOL( str ) \
    str.add( ", " ).addNewLine();


void GeoJSONWriter::writeGeometry( BufferString geomtyp,
			const coord2dset& crdset, const BufferStringSet& nms )
{
    geojsontree_->setProperties( properties_ );
    OD::GeoJsonTree::ValueSet* valueset = geojsontree_->createJSON( geomtyp,
						crdset, nms, coordsys_ );
    BufferString str;
    valueset->dumpJSon( str );
    strm() << str;
}


void GeoJSONWriter::writeGeometry( BufferString geomtyp,
			const coord3dset& crdset, const BufferStringSet& nms )
{
    geojsontree_->setProperties( properties_ );
    OD::GeoJsonTree::ValueSet* valueset = geojsontree_->createJSON( geomtyp,
						crdset, nms, coordsys_ );
    BufferString str;
    valueset->dumpJSon( str );
    strm() << str;
}


void GeoJSONWriter::writeGeometry( BufferString geomtyp,
					const pickset& picks )
{
    geojsontree_->setProperties( properties_ );
    OD::GeoJsonTree::ValueSet* valueset = geojsontree_->createJSON( geomtyp,
							picks, coordsys_ );
    BufferString str;
    valueset->dumpJSon( str );
    strm() << str;
}
