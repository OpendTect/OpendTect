/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Arnaud
 * DATE     : April 2018
-*/

#include "geojson.h"
#include "od_istream.h"
#include "latlong.h"
#include "survinfo.h"
#include "giswriter.h"
#include "coordsystem.h"


uiRetVal OD::GeoJsonTree::use( const char* fnm )
{
    od_istream strm( fnm );
    return use( strm );
}


uiRetVal OD::GeoJsonTree::use( od_istream& strm )
{
    filename_.set( strm.fileName() );

    uiRetVal uirv = read( strm );
    if ( !uirv.isOK() )
	return uirv;

    doGeoJSonCheck( uirv );
    return uirv;
}


#define mCheckPresence(obj,ky,fn) \
    const auto* ky = (obj)->fn( #ky ); \
    if ( !ky ) \
    { \
	uirv.set( missing_key_str.arg( #ky ).arg( filename_ ) ); \
	return; \
    }

void OD::GeoJsonTree::doGeoJSonCheck( uiRetVal& uirv )
{
    uirv.setEmpty();
    uiString missing_key_str( tr("Missing %1 key in GeoJSON file '%2'") );

    mCheckPresence( this, crs, getObject )
    mCheckPresence( crs, properties, getObject )
    mCheckPresence( this, features, getArray )
    if ( features->isEmpty() )
	uirv.set( tr("No features in GeoJSON file '%1'").arg( filename_ ) );
}


BufferString OD::GeoJsonTree::crsName() const
{
    return getObject( "crs" )
	 ->getObject( "properties" )
	 ->getStringValue( sKeyName() );
}


void  OD::GeoJsonTree::addCoord( const Coord3& coord, Array& poly )
{

    const LatLong ll( LatLong::transform(coord.getXY(), true, coordsys_) );
    if ( isfeatpoly_ )
    {
	Array* coordarr = poly.add( new Array(OD::JSON::Number) );
	coordarr->add( ll.lng_ ).add( ll.lat_ ).add( coord.z_ );
    }
    else
	poly.add( ll.lng_ ).add( ll.lat_ ).add( coord.z_ );
}


void  OD::GeoJsonTree::addCoord( const Coord& coord, Array& poly )
{
    const LatLong ll( LatLong::transform( coord, true, coordsys_ ) );
    if ( isfeatpoly_ )
    {
	Array* coordarr = poly.add( new Array(OD::JSON::Number) );
	coordarr->add( ll.lng_ ).add( ll.lat_ );
    }
    else
	poly.add( ll.lng_ ).add( ll.lat_ );
}


#define mCreateFeatArray(geomtyp) \
{ \
    isfeatpoint_ = geomtyp == "Point"; \
    isfeatpoly_ = (geomtyp == "Polygon")  || (geomtyp == "MultiPolygon"); \
    topobj_->set("type", "FeatureCollection"); \
    featarr_ = topobj_->set( "features", new Array(true) ); \
} \

#define mAddCoord \
for ( int idx=0; idx<nms.size(); idx++ ) \
{ \
    property_.objnm_ = *nms[idx]; \
    polyarr_ = createFeatCoordArray( featarr_, geomtyp ); \
    Array* poly(0); \
    if (isfeatpoly_) \
	poly = polyarr_->add( new Array( false ) ); \
    for( int cidx=0; cidx<crdset.size(); cidx++ ) \
    { \
	if (!isfeatpoint_) \
	{ \
	    if ( !isfeatpoly_ ) \
		poly = polyarr_->add( new Array( OD::JSON::Number ) ); \
	    addCoord( crdset[cidx], *poly ); \
	} \
	else \
	    addCoord( crdset[cidx], *polyarr_ ); \
    } \
} \


void OD::GeoJsonTree::setCRS( ConstRefMan<Coords::CoordSystem> crs )
{
    coordsys_ = crs;
    property_.coordysynm_ = crs.getNonConstPtr()->getURNString();
}


OD::GeoJsonTree::Object* OD::GeoJsonTree::createCRSArray( Array* crsarr )
{
    Object* featobj = crsarr->add( new Object );
    featobj->set( "type", "name" );

    Object* propobj = featobj->set( "properties", new Object );
    propobj->set( "name", property_.coordysynm_ );
    return propobj;
}


OD::GeoJsonTree::Array* OD::GeoJsonTree::createFeatCoordArray( Array* featarr,
	    BufferString typ )
{
    Object* featobj = featarr->add( new Object );
    featobj->set( "type", "Feature" );

    Object* propobj = featobj->set( "properties", new Object );
    propobj->set( "color", property_.color_.getStdStr(false, -1) );
    propobj->set( "style name", property_.stlnm_ );
    propobj->set( "width", property_.width_ );
    propobj->set( property_.nmkeystr_, property_.objnm_ );

    Object* crsobj = featobj->set( "crs", new Object );
    crsobj->set( "type", "name" );
    Object* crspropobj = crsobj->set( "properties", new Object );
    crspropobj->set( "name", property_.coordysynm_ );

    Object* geomobj = featobj->set( "geometry", new Object );
    geomobj->set( "type", typ );
    return geomobj->set( "coordinates", isfeatpoint_ ?
			    new Array(OD::JSON::Number) : new Array( false ) );
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
		const coord2dset& crdset, const BufferStringSet& nms,
		ConstRefMan<Coords::CoordSystem> crs )
{
    if ( topobj_->isEmpty() )
	mCreateFeatArray( geomtyp )

    setCRS( crs );

    mAddCoord;

    return topobj_->clone();
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
			const coord3dset& crdset, const BufferStringSet& nms,
			ConstRefMan<Coords::CoordSystem> crs )
{

    if (topobj_->isEmpty())
	mCreateFeatArray( geomtyp )

    setCRS( crs );

    mAddCoord;

    return topobj_->clone();
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
    const pickset& pckset, ConstRefMan<Coords::CoordSystem> crs )
{
    if (topobj_->isEmpty())
	mCreateFeatArray( geomtyp )

    setCRS( crs );

    for (int idx = 0; idx < pckset.size(); idx++)
    {
	const Pick::Set* pick = pckset.get( idx );
	property_.objnm_ = pick->name();
	polyarr_ = createFeatCoordArray( featarr_, geomtyp );
	coord2dset crdset;
	pick->getLocations( crdset );

	Array* poly(0);
	if ( isfeatpoly_ )
	    poly = polyarr_->add( new Array(false) );

	for (int cidx = 0; cidx < crdset.size(); cidx++)
	{
	    if ( !isfeatpoint_ )
	    {
		if ( !isfeatpoly_ )
		    poly = polyarr_->add( new Array(OD::JSON::Number) );
		addCoord( crdset[cidx], *poly );
	    }
	    else
		addCoord( crdset[cidx], *polyarr_ );
	}
    }

    return topobj_->clone();
}


bool OD::GeoJsonTree::isAntiMeridianCrossed( const coord3dset& crdset )
{
    for ( auto crd : crdset)
    {
	const LatLong ll( LatLong::transform( crd.getXY(), true, coordsys_) );
	if ( ll.lng_ < 0 )
	    break;
    }
    return true;
}


void OD::GeoJsonTree::setProperties( const GISWriter::Property& property )
{
    property_ = property;
}
