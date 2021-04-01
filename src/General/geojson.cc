/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : March 2021
-*/

#include "geojson.h"
#include "od_istream.h"
#include "latlong.h"
#include "picklocation.h"
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

    const LatLong ll( LatLong::transform(coord.coord(), true, coordsys_) );
    if ( isfeatpoly_ )
    {
	Array* coordarr = poly.add( new Array(OD::JSON::Number) );
	coordarr->add( ll.lng_ ).add( ll.lat_ ).add( coord.z );
    }
    else
	poly.add( ll.lng_ ).add( ll.lat_ ).add( coord.z );
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
    GISWriter::Property property; \
    property.objnm_ = *nms[idx]; \
    polyarr_ = createFeatCoordArray( featarr_, geomtyp, property ); \
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
}


OD::GeoJsonTree::Object* OD::GeoJsonTree::createCRSArray( Array* crsarr )
{
    Object* featobj = crsarr->add( new Object );
    featobj->set( "type", "name" );

    Object* propobj = featobj->set( "properties", new Object );
    propobj->set( "name", coordsys_->getURNString() );
    return propobj;
}


OD::GeoJsonTree::Array* OD::GeoJsonTree::createFeatCoordArray( Array* featarr,
	    BufferString typ , GISWriter::Property property )
{
    if ( !featarr )
	return nullptr;

    Object* featobj = featarr->add( new Object );
    featobj->set( "type", "Feature" );

    Object* styleobj = featobj->set( "style", new Object );
    const Color clr = property.color_;
    styleobj->set( "fill", clr.getStdStr(false, -1) );
    styleobj->set( "stroke-width", property.width_ );
    styleobj->set( "fill-opacity", clr.t() );

    Object* crsobj = featobj->set( "crs", new Object );
    crsobj->set( "type", "name" );
    Object* crspropobj = crsobj->set( "properties", new Object );
    crspropobj->set( "name", coordsys_->getURNString() );

    Object* geomobj = featobj->set( "geometry", new Object );
    geomobj->set( "type", typ );
    return geomobj->set( "coordinates", isfeatpoint_ ?
			    new Array(OD::JSON::Number) : new Array( false ) );
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
	    const coord2dset& crdset, const BufferStringSet& nms,
	    ConstRefMan<Coords::CoordSystem> crs, const BufferString& iconnm  )
{
    if ( topobj_->isEmpty() )
	mCreateFeatArray( geomtyp )

    setCRS( crs );

    mAddCoord;

    return topobj_->clone();
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
	    const coord3dset& crdset, const BufferStringSet& nms,
	    ConstRefMan<Coords::CoordSystem> crs, const BufferString& iconnm )
{

    if ( topobj_->isEmpty() )
	mCreateFeatArray( geomtyp )

    setCRS( crs );

    mAddCoord;

    return topobj_->clone();
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
    const pickset& pckset, ConstRefMan<Coords::CoordSystem> crs,
    const BufferString& iconnm )
{
    if ( topobj_->isEmpty() )
	mCreateFeatArray( geomtyp )

    setCRS( crs );

    for ( int idx=0; idx<pckset.size(); idx++ )
    {
	const Pick::Set* pick = pckset.get( idx );
	if ( !pick )
	    continue;

	GISWriter::Property property;
	property.color_ = pick->disp_.color_;
	property.width_ = pick->disp_.pixsize_*0.1;
	property.objnm_ = pick->name();
	property.iconnm_ = iconnm;
	polyarr_ = createFeatCoordArray( featarr_, geomtyp, property );
	ObjectSet<const Pick::Location> crdset;
	pick->getLocations( crdset );
	Array* poly(0);
	if ( isfeatpoly_ )
	    poly = polyarr_->add( new Array(false) );

	for ( int cidx=0; cidx<crdset.size(); cidx++ )
	{
	    if ( !isfeatpoint_ )
	    {
		if ( !isfeatpoly_ )
		    poly = polyarr_->add( new Array(OD::JSON::Number) );
		addCoord( crdset[cidx]->pos(), *poly );
	    }
	    else
		addCoord( crdset[cidx]->pos(), *polyarr_ );
	}
    }

    return topobj_->clone();
}


bool OD::GeoJsonTree::isAntiMeridianCrossed( const coord3dset& crdset )
{
    for ( auto crd : crdset)
    {
	const LatLong ll( LatLong::transform( crd.coord(), true, coordsys_) );
	if ( ll.lng_ < 0 )
	    break;
    }
    return true;
}


//void OD::GeoJsonTree::setProperties( const GISWriter::Property& property )
//{
//    property_ = property;
//}
