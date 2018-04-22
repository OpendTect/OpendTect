/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2016
-*/


#include "testprog.h"
#include "odjson.h"
#include "geometry.h"

using namespace OD::JSON;

static ValueSet* jsontree;

static const char* jsonstrs[] = {

// one good
"{ \"type\": \"FeatureCollection\", \"name\": \"3D Seismic\", \"root\": \"/auto/d43/surveys\", \"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" } }, \"features\": [ { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3NAM1982A\", \"name\": \"F3_Demo_d30\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 7, 55.0554844553, 0.0 ], [ 6, 55.0556671475, 0.0 ], [ 6, 54.9236026526, 0.0 ], [ 7, 54.9229699809, 0.0 ], [ 7, 55.0554844553, 0.0 ] ] ] } }, { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3GDF2010A\", \"name\": \"MagellanesBasin\", \"the_answer_to_everything\": 42 }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 9, 53.8820024932, 0.0 ], [ 7, 53.877063624,  0.0 ], [ 7, 53.7857242316, 0.0 ], [ 8, 53.7419173548, 0.0 ], [ 9, 53.7461123222, 0.0 ], [ 9, 53.8820024932, 0.0 ] ] ] } } ] }",

// any number of bad ones
"\"aap\": \"noot\"]",
"{\"aap\": \"noot\"",
"{\"aap\": \"noot\"]",
"{ x: \"y\"}",

0
};


static bool testParseJSON()
{
    BufferStringSet bss( jsonstrs );
    int idx = 0;
    for ( auto strptr : bss )
    {
	BufferString& str = *strptr;

	uiRetVal uirv;
	ValueSet* tree = ValueSet::parseJSon( str.getCStr(), str.size(), uirv );
	const bool isok = uirv.isOK();
	mRunStandardTest( isok==(idx == 0), BufferString("Parse result ",idx) )
	if ( !quiet && !isok )
	    tstStream() << "\tmsg=" << toString(uirv) << od_endl;

	if ( isok )
	    jsontree = tree;
	idx++;
    }

    return true;
}


#define mCheckNonNull( ptr, nm ) \
    mRunStandardTest( ptr != 0, #nm " present" )


static bool testUseJSON( bool created )
{
    const Node& tree = *static_cast<const Node*>( jsontree );
    tstStream() << "\n\nUse JSON, " << (created ? "Created." : "Original.")
		<< od_endl;

    const auto* crsnode = tree.getNode( "crs" );
    mCheckNonNull( crsnode, crs );
    const auto* crspropnode = crsnode->getNode( "properties" );
    mCheckNonNull( crspropnode, crs.properties );
    BufferString namestr = crspropnode->getStringValue( "name" );
    mRunStandardTest( namestr=="urn:ogc:def:crs:OGC:1.3:CRS84", "crs name" );

    const auto* featsarr = tree.getArray( "features" );
    mCheckNonNull( featsarr, features );
    const int sz = featsarr->size();
    mRunStandardTest( sz==2, "Number of features" );
    const auto& feat2 = featsarr->node( 1 );

    const auto* feat2props = feat2.getNode( "properties" );
    mCheckNonNull( feat2props, feats[1].props );
    namestr = feat2props->getStringValue( "name" );
    mRunStandardTest( namestr=="MagellanesBasin", "props.name" );
    if ( !created )
    {
	od_int64 intval = feat2props->getIntValue( "the_answer_to_nothing" );
	mRunStandardTest( mIsUdf(intval), "props.the_answer_to_nothing" );
	intval = feat2props->getIntValue( "the_answer_to_everything" );
	mRunStandardTest( intval==42, "props.the_answer_to_everything" );
    }

    const auto* geomnode = feat2.getNode( "geometry" );
    mCheckNonNull( geomnode, feat2.geometry );
    BufferString typestr = geomnode->getStringValue( "type" );
    mRunStandardTest( typestr=="Polygon", "geometry.type" );

    const auto* coords = geomnode->getArray( "coordinates" );
    mCheckNonNull( coords, geometry.coordinates );
    const int nrpolys = coords->size();
    mRunStandardTestWithError( nrpolys==1, "Number of polys",
			       BufferString("nrpolys=",nrpolys) );

    const auto& poly = coords->array( 0 );
    const int nrcoords = poly.size();
    mRunStandardTestWithError( nrcoords==6, "Number of coordinates",
			       BufferString("nrcoords=",nrcoords) );

    const auto& point3 = poly.array( 3 );
    const int coordsz = point3.size();
    mRunStandardTestWithError( coordsz==3, "size of coords",
			       BufferString("coordsz=",coordsz) );
    mRunStandardTest( point3.valType()==OD::JSON::ValueSet::Data,
			       "array type" );

    const auto& val_ts = point3.valArr().vals();
    const Coord3 coord( val_ts[0], val_ts[1], val_ts[2] );
    mRunStandardTest( coord==Coord3(8,53.7419173548,0.0), "coordinate value" );

    return true;
}


static Array* createFeatCoordArray( Array* featarr,
	const char* id, const char* nm, const char* typ )
{
    Node* featnode = featarr->add( new Node );
    featnode->set( "type", "Feature" );

    Node* propnode = featnode->set( "properties", new Node );
    propnode->set( "id", id );
    propnode->set( "name", nm );

    Node* geomnode = featnode->set( "geometry", new Node );
    geomnode->set( "type", typ );

    return geomnode->set( "coordinates", new Array(false) );
}


static void addCoords( const TypeSet<Coord3>& coords, Array& poly )
{
    for ( const auto& coord : coords )
    {
	Array* coordarr = poly.add( new Array(Number) );
	coordarr->add( coord.x_ ).add( coord.y_ ).add( coord.z_ );
    }
}


static bool testCreateJSON()
{
    Node& topnode = *new Node;

    topnode.set( "type", "FeatureCollection" );
    topnode.set( "name", "3D Seismic" );
    topnode.set( "root", "/auto/d43/surveys" );

    Node* crsnode = topnode.set( "crs", new Node );
    crsnode->set( "type", "name" );
    Node* crspropsnode = crsnode->set( "properties", new Node );
    crspropsnode->set( "name", "urn:ogc:def:crs:OGC:1.3:CRS84" );

    Array* featarr = topnode.set( "features", new Array(true) );
    Array* polyarr = createFeatCoordArray( featarr, "Z3NAM1982A", "F3_Demo_d30",
					   "Polygon" );
    Array* poly = polyarr->add( new Array(false) );

    // Once via TypeSet<double> for test
    Array* coord = poly->add( new Array(Number) );
    TypeSet<double> cvals;
    cvals += 7; cvals += 55.0554844553; cvals += 0.0;
    coord->set( cvals );

    // Once via TypeSet for test
    TypeSet<Coord3> coords;
    coords += Coord3( 6, 55.0556671475, 0.0 );
    coords += Coord3( 6, 54.9236026526, 0.0 );
    coords += Coord3( 7, 54.9229699809, 0.0 );
    coords += Coord3( 7, 55.0554844553, 0.0 );
    addCoords( coords, *poly );

    polyarr = createFeatCoordArray( featarr, "Z3GDF2010A", "MagellanesBasin",
					   "Polygon" );
    poly = polyarr->add( new Array(false) );
    coords.setEmpty();
    coords += Coord3( 9, 53.8820024932, 0.0 );
    coords += Coord3( 7, 53.877063624,  0.0 );
    coords += Coord3( 7, 53.7857242316, 0.0 );
    coords += Coord3( 8, 53.7419173548, 0.0 );
    coords += Coord3( 9, 53.7461123222, 0.0 );
    coords += Coord3( 9, 53.8820024932, 0.0 );
    addCoords( coords, *poly );

    delete jsontree;
    jsontree = &topnode;
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testParseJSON()
      || !testUseJSON(false)
      || !testCreateJSON()
      || !testUseJSON(true) )
	return 1;

    return 0;
}
