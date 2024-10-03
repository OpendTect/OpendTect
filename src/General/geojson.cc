/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geojson.h"

#include "giswriter.h"
#include "od_istream.h"
#include "picklocation.h"
#include "pickset.h"
#include "survinfo.h"


OD::JSON::GeoJsonTree::GeoJsonTree()
    : Object(nullptr)
    , inpcrs_(SI().getCoordSystem())
    , coordsys_(Coords::CoordSystem::getWGS84LLSystem())
{}


OD::JSON::GeoJsonTree::~GeoJsonTree()
{
}


uiRetVal OD::JSON::GeoJsonTree::use( const char* fnm )
{
    od_istream strm( fnm );
    return use( strm );
}


uiRetVal OD::JSON::GeoJsonTree::use( od_istream& strm )
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

void OD::JSON::GeoJsonTree::doGeoJSonCheck( uiRetVal& uirv )
{
    uirv.setEmpty();
    uiString missing_key_str( tr("Missing %1 key in GeoJSON file '%2'") );

    mCheckPresence( this, features, getArray )
    if ( features->isEmpty() )
	uirv.set( tr("No features in GeoJSON file '%1'").arg( filename_ ) );
}


void OD::JSON::GeoJsonTree::setInputCoordSys( const Coords::CoordSystem* crs )
{
    inpcrs_ = crs;
}


void OD::JSON::GeoJsonTree::createFeatArray()
{
    set( "type", "FeatureCollection" );
    featarr_ = set( "features", new Array(true) );
}


OD::JSON::Array*
OD::JSON::GeoJsonTree::createFeatCoordArray( const GIS::Property& props )
{
    if ( isEmpty() )
	createFeatArray();

    if ( !featarr_ )
	return nullptr;

    Object* featobj = featarr_->add( new Object );
    featobj->set( "type", "Feature" );

    Object* propobj = featobj->set( "properties", new Object );
    const OD::String& objname = props.name();
    if ( !objname.isEmpty() && objname != "NONE" )
	propobj->set( "name", objname.str() );

    if ( props.isPoint() )
    {
	propobj->set( "marker-color", props.color_.getStdStr() );
	propobj->set( "marker-size", props.pixsize_ < 2
			? "small" :(props.pixsize_ > 2 ? "large" : "medium") );
	propobj->set( "marker-symbol", props.iconnm_ );
    }
    else
    {
	const OD::LineStyle& linestyle = props.linestyle_;
	propobj->set( "stroke", linestyle.color_.getStdStr() );
	propobj->set( "stroke-width", linestyle.width_);
	propobj->set( "stroke-opacity", (1.-linestyle.color_.tF()) );
	if ( props.isPolygon() )
	{
	    const OD::Color& fillcolor = props.fillcolor_;
	    propobj->set( "fill", fillcolor.getStdStr() );
	    propobj->set( "fill-opacity", (1.-fillcolor.tF()) );
	}
    }

    Object* geomobj = featobj->set( "geometry", new Object );
    geomobj->set( "type", GIS::FeatureTypeDef().toString(props.type_));
    if ( props.isPoint() && !props.isMulti() )
	return geomobj->set( "coordinates", new Array(Number) );

    Array* geomarr = geomobj->set( "coordinates", new Array(false) );
    if ( props.isPolygon() )
	geomarr = geomarr->add( new Array(false) );

    return geomarr;
}


bool OD::JSON::GeoJsonTree::addPoint( const Coord& crd,
				      const GIS::Property& properties )
{
    const LatLong ll = LatLong::transform( crd, true, inpcrs_ );
    return addPoint( ll, mUdf(double), properties );
}


bool OD::JSON::GeoJsonTree::addPoint( const Coord3& crd,
				      const GIS::Property& properties )
{
    const LatLong ll = LatLong::transform( crd.coord(), true, inpcrs_ );
    return addPoint( ll, crd.z_, properties );
}


bool OD::JSON::GeoJsonTree::addPoint( const LatLong& ll, double z,
				      const GIS::Property& properties )
{
    Array* coordarr = createFeatCoordArray( properties );
    if ( !coordarr )
	return false;

    addLatLong( ll, z, *coordarr );
    return true;
}


bool OD::JSON::GeoJsonTree::addFeatures( const TypeSet<Coord>& coords,
					 const GIS::Property& properties )
{
    if ( coords.isEmpty() )
	return false;

    Array* coordarr = createFeatCoordArray( properties );
    if ( !coordarr )
	return false;

    if ( properties.isMulti() && !properties.isPoint() )
    {
	pErrMsg("MultiLineString and MultiPolygon feature type are only "
		"supported via a multi-set Pick::Set object");
    }

    ConstRefMan<Coords::CoordSystem> inpcrs = inpcrs_.ptr();
    const Coords::CoordSystem* crs = inpcrs.ptr();
    for ( const auto& crd : coords )
    {
	Array* posarr = coordarr->add( new Array(Number) );
	if ( posarr )
	    addCoord( crd, crs, *posarr );
    }

    if ( properties.isPolygon() && coords.first() != coords.last() )
    {
	Array* posarr = coordarr->add( new Array(Number) );
	if ( posarr )
	    addCoord( coords.first(), crs, *posarr );
    }

    return true;
}


bool OD::JSON::GeoJsonTree::addFeatures( const TypeSet<Coord3>& coords,
					 const GIS::Property& properties )
{
    if ( coords.isEmpty() )
	return false;

    Array* coordarr = createFeatCoordArray( properties );
    if ( !coordarr )
	return false;

    if ( properties.isMulti() && !properties.isPoint() )
    {
	pErrMsg("MultiLineString and MultiPolygon feature types are only "
		"supported via a multi-set Pick::Set object");
    }

    ConstRefMan<Coords::CoordSystem> inpcrs = inpcrs_.ptr();
    const Coords::CoordSystem* crs = inpcrs.ptr();
    for ( const auto& crd : coords )
    {
	Array* posarr = coordarr->add( new Array(Number) );
	if ( posarr )
	    addCoord( crd, crs, *posarr );
    }

    if ( properties.isPolygon() && coords.first() != coords.last() )
    {
	Array* posarr = coordarr->add( new Array(Number) );
	if ( posarr )
	    addCoord( coords.first(), crs, *posarr );
    }

    return true;
}


bool OD::JSON::GeoJsonTree::addFeatures( const Pick::Set& pickset,
					 const GIS::Property& properties )
{
    if ( pickset.isEmpty() )
	return false;

    Array* coordarr = createFeatCoordArray( properties );
    if ( !coordarr )
	return false;

    ConstRefMan<Coords::CoordSystem> inpcrs = inpcrs_.ptr();
    const Coords::CoordSystem* crs = inpcrs.ptr();
    const int totsz = pickset.size();
    const int nrsets = pickset.nrSets();
    if ( pickset.nrSets() < 2 )
    {
	for ( int idx=0; idx<totsz; idx++ )
	{
	    Array* posarr = coordarr->add( new Array(Number) );
	    if ( posarr )
		addCoord( pickset.getPos(idx), crs, *posarr );
	}

	if ( properties.isPolygon() &&
	     pickset.getPos(0) != pickset.getPos(totsz-1) )
	{
	    Array* posarr = coordarr->add( new Array(Number) );
	    if ( posarr )
		addCoord( pickset.getPos(0), crs, *posarr );
	}
    }
    else
    {
	for ( int iset=0; iset<nrsets; iset++ )
	{
	    Array* subarr = coordarr->add( new Array(false) );
	    int start, stop;
	    pickset.getStartStopIdx( iset, start, stop );
	    for ( int idx=start; idx<stop; idx++ )
	    {
		Array* posarr = subarr->add( new Array(Number) );
		if ( posarr )
		    addCoord( pickset.getPos(idx), crs, *posarr );
	    }

	    const Coord3& firstpos = pickset.getPos( start );
	    const Coord3& lastpos = pickset.getPos( stop );
	    if ( properties.isPolygon() && firstpos != lastpos )
	    {
		Array* posarr = subarr->add( new Array(Number) );
		if ( posarr )
		    addCoord( firstpos, crs, *posarr );
	    }
	}
    }

    return true;
}


void OD::JSON::GeoJsonTree::addLatLong( const LatLong& ll, double z,
					Array& coordarr )
{
    coordarr.add( ll.lng_ ).add( ll.lat_ );
    if ( !mIsUdf(z) )
	coordarr.add( z );
}


void OD::JSON::GeoJsonTree::addCoord( const Coord& coord,
			    const Coords::CoordSystem* crs, Array& coordarr )
{
    const LatLong ll = LatLong::transform( coord, true, crs );
    return addLatLong( ll, mUdf(double), coordarr );
}


void OD::JSON::GeoJsonTree::addCoord( const Coord3& coord,
			    const Coords::CoordSystem* crs, Array& coordarr )
{
    const LatLong ll = LatLong::transform( coord.coord(), true, crs );
    return addLatLong( ll, coord.z_, coordarr );
}
