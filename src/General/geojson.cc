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


void  OD::GeoJsonTree::addCoords( const TypeSet<Coord3>& coords, Array& poly )
{

    for ( const auto& coord : coords )
    {
	const LatLong ll( LatLong::transform( coord.getXY(), true,
					     SI().getCoordSystem()) );
	if ( !isfeatpoint_ )
	{
	    Array* coordarr = poly.add( new Array(OD::JSON::Number) );
	    coordarr->add( ll.lng_ ).add( ll.lat_ ).add( coord.z_ );
	}
	else
	    poly.add( ll.lng_ ).add( ll.lat_ ).add( coord.z_ );
    }
}


void  OD::GeoJsonTree::addCoords( const TypeSet<Coord>& coords, Array& poly )
{
    for ( const auto& coord : coords )
    {
	const LatLong ll( LatLong::transform( coord, true,
					     SI().getCoordSystem()) );
	if ( !isfeatpoint_ )
	{
	    Array* coordarr = poly.add( new Array(OD::JSON::Number) );
	    coordarr->add( ll.lng_ ).add( ll.lat_ );
	}
	else
	    poly.add( ll.lng_ ).add( ll.lat_ );
    }
}


#define mCreateFeatArray(geomtyp) \
    isfeatpoint_ = geomtyp == "Point"; \
    isfeatpoly_ = (geomtyp == "Polygon")  || (geomtyp == "MultiPolygon"); \
    Object topobj;  \
    topobj.set("type", "FeatureCollection"); \
    Array* featarr = topobj.set("features", new Array(true)); \


OD::GeoJsonTree::Array* OD::GeoJsonTree::createFeatCoordArray( Array* featarr,
	    BufferString typ )
{
    Object* featobj = featarr->add( new Object );
    featobj->set( "type", "Feature" );

    Object* propobj = featobj->set( "properties", new Object );
    propobj->set( "color", property_.color_.getStdStr(false, -1) );
    propobj->set( "style name", property_.stlnm_ );
    propobj->set( "width", property_.width_ );


    Object* geomobj = featobj->set( "geometry", new Object );
    geomobj->set( "type", typ );
    return geomobj->set( "coordinates", isfeatpoint_ ?
			    new Array(OD::JSON::Number) : new Array(false) );
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
					const coord2dset& crdset )
{
    mCreateFeatArray( geomtyp )

    Array* polyarr = createFeatCoordArray( featarr, geomtyp );

    if (isfeatpoly_)
    {
	Array* poly = polyarr->add(new Array(false));
	addCoords(crdset, *poly);
    }
    else
	addCoords(crdset, *polyarr);

    return topobj.clone();
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
						const coord3dset& crdset )
{
    mCreateFeatArray( geomtyp )

    Array* polyarr = createFeatCoordArray( featarr, geomtyp );

    if (isfeatpoly_)
    {
	Array* poly = polyarr->add(new Array(false));
	addCoords(crdset, *poly);
    }
    else
	addCoords(crdset, *polyarr);

    return topobj.clone();
}


bool OD::GeoJsonTree::isAntiMeridianCrossed( const coord3dset& crdset )
{
    for ( auto crd : crdset)
    {
	const LatLong ll( LatLong::transform( crd.getXY(), true,
					    SI().getCoordSystem()) );
	if ( ll.lng_ < 0 )
	    break;
    }
    return true;
}


void OD::GeoJsonTree::setProperties( const GISWriter::Property& property )
{
    property_ = property;
}
