/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bart de Groot
 * DATE     : March 2018
-*/


#include "testprog.h"

#include "bufstring.h"
#include "coord.h"
#include "geojson.h"
#include "uistring.h"


using namespace OD;


static const char* jsonstr =

"{ \"type\": \"FeatureCollection\", \"name\": \"3D Seismic\", \"root\": \"/auto/d43/surveys\", \"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" } }, \"features\": [ { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3NAM1982A\", \"name\": \"F3_Demo_d30\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 7, 55.0554844553, 0.0 ], [ 6, 55.0556671475, 0.0 ], [ 6, 54.9236026526, 0.0 ], [ 7, 54.9229699809, 0.0 ], [ 7, 55.0554844553, 0.0 ] ] ] } }, { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3GDF2010A\", \"name\": \"MagellanesBasin\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 9, 53.8820024932, 0.0 ], [ 7, 53.877063624,  0.0 ], [ 7, 53.7857242316, 0.0 ], [ 8, 53.7419173548, 0.0 ], [ 9, 53.7461123222, 0.0 ], [ 9, 53.8820024932, 0.0 ] ] ] } } ] }";


static BufferString workstr( jsonstr );
static GeoJsonTree collection;
static ManagedObjectSet<ObjectSet<const TypeSet<Coord3> > > polygons;


static bool testParseCollection()
{
    uiRetVal uirv = collection.parseJSon( workstr.getCStr(), workstr.size() );
    const bool isok = uirv.isOK() && !collection.isEmpty();
    if ( !uirv.isOK() )
	logStream() << "\tmsg=" << toString(uirv) << od_endl;
    else if ( collection.isEmpty() )
	logStream() << "\tmsg=Empty FeatureCollection" << od_endl;

    mRunStandardTest( isok, "GeoJSon parses fine" )

    return true;
}


static bool testGeoJSonHeaders()
{
    const BufferString collectiontype(
		       collection.getStringValue( GeoJsonTree::sKeyType() ) );
    const bool cond1 = collectiontype == "FeatureCollection";
    const bool cond2 = collection.isPresent( GeoJsonTree::sKeyType() );
    const bool cond3 = collection.isPresent( GeoJsonTree::sKeyName() );
    const bool isok = cond1 && cond2 && cond3;
    if ( !cond1 )
	tstStream() << "\tmsg=Not a GeoJson collection" << od_endl;
    else if ( !cond2 || !cond3 )
    {
	tstStream() << "\tmsg=FeatureCollection does not have required header";
	tstStream() << od_endl;
    }

    mRunStandardTest( isok, "GeoJSon has standard main headers" )

    return true;
}


static bool testGeoJSonOptHeaders()
{
    const auto* crsobj = collection.getObject( GeoJsonTree::sKeyCRS() );
    if ( !crsobj )
    {
	mRunStandardTest( true, "GeoJSon does not have optional headers" )
	return true;
    }

    if ( !crsobj->isPresent(GeoJsonTree::sKeyType()) ||
	 !crsobj->isPresent(GeoJsonTree::sKeyProperties()) )
	mRunStandardTest( false, "GeoJSon optional feature is not usable" )

    const auto* crspropobj = crsobj->getObject( GeoJsonTree::sKeyProperties() );
    if ( !crspropobj )
	mRunStandardTest( false, "GeoJSon optional feature is not usable" )

    const BufferString crsnamestr(
		       crspropobj->getStringValue(GeoJsonTree::sKeyName()) );
    mRunStandardTest( crsnamestr == "urn:ogc:def:crs:OGC:1.3:CRS84",
		      "GeoJSon CRS string parses fine" )

    return true;
}


static bool testGeoJSonFeatureProps( const JSON::Object& feature )
{
    const auto* featurepropobj =
			 feature.getObject( GeoJsonTree::sKeyProperties() );
    if ( !featurepropobj )
	mRunStandardTest( false, "Cannot parse GeoJSon feature properties" )

    const BufferString featsurvnm(
		  featurepropobj->getStringValue(GeoJsonTree::sKeyName()) );
    const bool goodsurvnm = featsurvnm == "F3_Demo_d30" ||
			    featsurvnm == "MagellanesBasin";
    mRunStandardTest( goodsurvnm, "GeoJSon survey name parsing" )

    const BufferString featsurvid(
		  featurepropobj->getStringValue("id") );
    const bool goodsurvid = featsurvid == "Z3NAM1982A" ||
			    featsurvid == "Z3GDF2010A";
    mRunStandardTest( goodsurvid, "GeoJSon survey ID parsing" )

    return true;
}


static bool testGeoJSonFeature( const JSON::Object& feature )
{
    if ( !feature.isPresent(GeoJsonTree::sKeyType()) ||
	 !feature.isPresent(GeoJsonTree::sKeyProperties()) ||
	 !feature.isPresent(GeoJsonTree::sKeyGeometry()) )
	mRunStandardTest( false, "Invalid GeoJSon feature" )

    const BufferString featnamestr(
			  feature.getStringValue(GeoJsonTree::sKeyType()) );
    if ( featnamestr != "Feature" )
	mRunStandardTest( false, "Incorrect object type in features list" )

    return testGeoJSonFeatureProps( feature );
}


static bool testExtractCoord( const JSON::Array& point3 )
{
    if ( point3.size() < 3 ||
	 point3.valType() != OD::JSON::ValueSet::Data )
	mRunStandardTest( false, "Incorrect feature position in collection" )

    return true;
}


static bool testExtractCoordinates( const JSON::Array& coordsarr )
{
    const JSON::ValueSet::size_type arrsz = coordsarr.size();
    ObjectSet<const TypeSet<Coord3> >* survpolygons =
			    new ManagedObjectSet<const TypeSet<Coord3> >;
    polygons.add( survpolygons );
    for ( JSON::ValueSet::size_type idx=0; idx<arrsz; idx++ )
    {
	const auto& polygonsarr = coordsarr.array( idx );
	const JSON::ValueSet::size_type polygonsarrsz = polygonsarr.size();
	TypeSet<Coord3> poslist( polygonsarrsz, Coord3::udf() );
	for ( JSON::ValueSet::size_type idy=0; idy<polygonsarrsz; idy++ )
	{
	    const auto& point3 = polygonsarr.array( idy );
	    if ( !testExtractCoord(point3) )
		return false;

	    const auto& val_ts = point3.valArr().vals();
	    poslist[idy] = Coord3( val_ts[0], val_ts[1], val_ts[2] );
	}

	survpolygons->add( new TypeSet<Coord3>( poslist) );
    }

    return true;
}


static bool testGeoJSonFeaturePosExtraction( const JSON::Object& feature )
{
    const auto* featuregeomobj =
			 feature.getObject( GeoJsonTree::sKeyGeometry() );
    //presence already tested
    if ( !featuregeomobj->isPresent(GeoJsonTree::sKeyType()) ||
	 !featuregeomobj->isPresent(GeoJsonTree::sKeyCoord()) )
	mRunStandardTest( false, "GeoJSon feature is not recognized" )

    const BufferString featgeom(
		featuregeomobj->getStringValue( GeoJsonTree::sKeyType()));
    if ( featgeom != "Polygon" )
	mRunStandardTest( false, "GeoJSon feature is not a polygon" )

    return testExtractCoordinates(
			*featuregeomobj->getArray( GeoJsonTree::sKeyCoord() ) );
}


static bool testGeoJSonFeatureCollection()
{
    const auto* features =
			collection.getArray( GeoJsonTree::sKeyFeatures() );
    if ( !features )
	mRunStandardTest( false, "GeoJSon does not have an array of features" )

    const JSON::ValueSet::size_type nrfeatures = features->size();
    polygons.setEmpty();
    for ( JSON::ValueSet::size_type ifeat=0; ifeat<nrfeatures; ifeat++ )
    {
	if ( features->valueType(ifeat) != JSON::ValueSet::SubObject )
	{
	    mRunStandardTest( features,
			      "GeoJSon does not have valid Feature object" )
	}

	const auto& feature = features->object( ifeat );
	if ( !testGeoJSonFeature(feature) ||
	     !testGeoJSonFeaturePosExtraction(feature) )
	    return false;
    }

    return true;
}


static bool testGeoJSonExtractedData()
{
    const int nrsurveys = polygons.size();
    if ( nrsurveys != 2 )
	mRunStandardTest( false, "Not all polygons have been parsed" )

    int nrpolygons = 0;
    int nrpoints = 0;
    for ( auto survpolygons : polygons )
    {
	for ( auto polygon : *survpolygons )
	{
	    nrpolygons++;
	    for ( auto point3d : *polygon )
	    {
		if ( point3d.isDefined() )
		    nrpoints++;
	    }
	}
    }

    if ( nrpolygons != 2 )
	mRunStandardTest( false, "Expected 2 polygons" )

    if ( nrpoints != 11 )
	mRunStandardTest( false, "Expected 11 points upon all polygons" )

    mRunStandardTest( true, "Parsed all polygons correctly from GeoJSon" )

    return true;
}



int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testParseCollection() ||
	 !testGeoJSonHeaders() ||
	 !testGeoJSonOptHeaders() ||
	 !testGeoJSonFeatureCollection() ||
	 !testGeoJSonExtractedData() )
	return 1;

    return 0;
}
