/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bart de Groot
 * DATE     : March 2018
-*/


#include "testprog.h"

#include "bufstring.h"
#include "geojson.h"
#include "uistring.h"


using namespace OD;


static const char* jsonstr =

"{ \"type\": \"FeatureCollection\", \"name\": \"3D Seismic\", \"root\": \"/auto/d43/surveys\", \"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" } }, \"features\": [ { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3NAM1982A\", \"name\": \"F3_Demo_d30\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 7, 55.0554844553, 0.0 ], [ 6, 55.0556671475, 0.0 ], [ 6, 54.9236026526, 0.0 ], [ 7, 54.9229699809, 0.0 ], [ 7, 55.0554844553, 0.0 ] ] ] } }, { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3GDF2010A\", \"name\": \"MagellanesBasin\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 9, 53.8820024932, 0.0 ], [ 7, 53.877063624,  0.0 ], [ 7, 53.7857242316, 0.0 ], [ 8, 53.7419173548, 0.0 ], [ 9, 53.7461123222, 0.0 ], [ 9, 53.8820024932, 0.0 ] ] ] } } ] }";


static BufferString workstr( jsonstr );



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    GeoJsonTree collection;
    uiRetVal uirv = collection.parseJSon( workstr.getCStr(), workstr.size() );
    if ( !quiet && !uirv.isOK() )
    {
	tstStream() << "\tmsg=" << toString(uirv) << od_endl;
	return 1;
    }

    if ( collection.isEmpty() )
    {
	tstStream() << "\tmsg=Empty FeatureCollection" << od_endl;
	return 1;
    }

    if ( !collection.isPresent(GeoJsonTree::sKeyType()) ||
	 !collection.isPresent(GeoJsonTree::sKeyName()) )
    {
	tstStream() << "\tmsg=FeatureCollection does not have required header";
	tstStream() << od_endl;
	return 1;
    }


    const BufferString collectiontype(
		       collection.getStringValue( GeoJsonTree::sKeyType() ) );
    if ( collectiontype != "FeatureCollection" )
    {
	tstStream() << "\tmsg=Not a GeoJson collection" << od_endl;
	return 1;
    }

    const auto* crsobj = collection.getObject( GeoJsonTree::sKeyCRS() );
    if ( !crsobj )
    {
	tstStream() << "\tmsg=Not a GeoJson collection" << od_endl;
	return 1;
    }

    if ( !crsobj->isPresent(GeoJsonTree::sKeyType()) ||
	 !crsobj->isPresent(GeoJsonTree::sKeyProperties()) )
    {
	tstStream() << "\tmsg=CRS object is incorrect" << od_endl;
	return 1;
    }

    const auto* crspropobj = crsobj->getObject( GeoJsonTree::sKeyProperties() );
    const BufferString crsnamestr(
		       crspropobj->getStringValue(GeoJsonTree::sKeyName()) );


    const auto* featureslist =
			collection.getArray( GeoJsonTree::sKeyFeatures() );
    if ( !featureslist )
    {
	tstStream() << "\tmsg=Not a GeoJson collection" << od_endl;
	return 1;
    }

    const JSON::ValueSet::size_type nrfeats = featureslist->size();
//    for ( auto feat : *featureslist )
    for ( JSON::ValueSet::size_type idx=0; idx<nrfeats; idx++ )
    {
	if ( featureslist->valueType(idx) != JSON::ValueSet::SubObject )
	{
	    tstStream() << "\tmsg=Incorrect feature in collection" << od_endl;
	    return 1;
	}

/*	const JSON::ValueSet& feature = featureslist->child( idx );
	const bool testfeature = false; //Type + property + geometry*/
    }

    return 0;
}
