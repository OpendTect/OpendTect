/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geojson.h"
#include "od_istream.h"
#include "latlong.h"
#include "picklocation.h"
#include "survinfo.h"
#include "giswriter.h"
#include "coordsystem.h"

OD::GeoJsonTree::GeoJsonTree()
    : Object(nullptr)
{}


OD::GeoJsonTree::GeoJsonTree( const GeoJsonTree& oth )
    : Object(oth)
    , filename_(oth.filename_)
{}


OD::GeoJsonTree::GeoJsonTree( const Object& obj )
    : Object(obj)
{}


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

#define mAddCoordWithProperty(property) \
for ( int idx=0; idx<nms.size(); idx++ ) \
{ \
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

    const Coords::CoordSystem::StringType styp = Coords::CoordSystem::URN;
    //TODO: Switch to URL

    Object* propobj = featobj->set( "properties", new Object );
    propobj->set( "name", coordsys_->getDescString(styp) );
    return propobj;
}


OD::GeoJsonTree::Array* OD::GeoJsonTree::createFeatCoordArray( Array* featarr,
	    BufferString typ , GISWriter::Property property )
{
    if ( !featarr )
	return nullptr;

    Object* featobj = featarr->add( new Object );
    featobj->set( "type", "Feature" );

    Object* styleobj = featobj->set( "properties", new Object );
    const Color clr = property.color_;
    styleobj->set( "fill", clr.getStdStr() );
    styleobj->set( "stroke", clr.getStdStr() );
    styleobj->set( "stroke-width", property.width_==0 ? 2 : property.width_ );
    styleobj->set( "fill-opacity", clr.t() );

    const Coords::CoordSystem::StringType styp = Coords::CoordSystem::URN;
    //TODO: Switch to URL

    Object* crsobj = featobj->set( "crs", new Object );
    crsobj->set( "type", "name" );
    Object* crspropobj = crsobj->set( "properties", new Object );
    crspropobj->set( "name", coordsys_->getDescString(styp) );

    Object* geomobj = featobj->set( "geometry", new Object );
    geomobj->set( "type", typ );
    return geomobj->set( "coordinates", isfeatpoint_ ?
			    new Array(OD::JSON::Number) : new Array( false ) );
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
    const TypeSet<Coord>& crdset, const BufferStringSet& nms,
    ConstRefMan<Coords::CoordSystem> crs, GISWriter::Property& property )
{
    if ( topobj_->isEmpty() )
	mCreateFeatArray( geomtyp )

    setCRS( crs );

    mAddCoordWithProperty( property );

    return topobj_->clone();
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
	    const TypeSet<Coord3>& crdset, const BufferStringSet& nms,
	    ConstRefMan<Coords::CoordSystem> crs,
	    GISWriter::Property& property )
{

    if ( topobj_->isEmpty() )
	mCreateFeatArray( geomtyp )

    setCRS( crs );

    mAddCoordWithProperty( property );

    return topobj_->clone();
}


OD::GeoJsonTree::ValueSet* OD::GeoJsonTree::createJSON( BufferString geomtyp,
		const RefObjectSet<const Pick::Set>& pckset,
		ConstRefMan<Coords::CoordSystem> crs,
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

	if ( !iconnm.isEmpty() )
	    property.iconnm_ = iconnm;

	polyarr_ = createFeatCoordArray( featarr_, geomtyp, property );
	const TypeSet<Pick::Location>& locations = pick->locations();
	if ( pick->isPolygon() && pick->disp_.connect_ == pick->disp_.Close &&
	     locations.first() != locations.last() )
	{
	    auto& nclocs = cCast(TypeSet<Pick::Location>&,locations);
	    nclocs.add( nclocs.first() );
	}

	Array* poly(0);
	if ( isfeatpoly_ )
	    poly = polyarr_->add( new Array(false) );

	for ( int cidx=0; cidx<locations.size(); cidx++ )
	{
	    if ( !isfeatpoint_ )
	    {
		if ( !isfeatpoly_ )
		    poly = polyarr_->add( new Array(OD::JSON::Number) );

		addCoord( locations.get(cidx).pos(), *poly );
	    }
	    else
		addCoord( locations.get(cidx).pos(), *polyarr_ );
	}
    }

    return topobj_->clone();
}


bool OD::GeoJsonTree::isAntiMeridianCrossed( const TypeSet<Coord3>& crdset )
{
    for ( auto crd : crdset)
    {
	const LatLong ll( LatLong::transform( crd.coord(), true, coordsys_) );
	if ( ll.lng_ < 0 )
	    break;
    }
    return true;
}
