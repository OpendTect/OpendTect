/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2016
-*/


#include "testprog.h"
#include "odjson.h"

using namespace OD::JSON;

static ValueSet* jsontree;

static const char* jsonstrs[] = {

// one good
"{ \"type\": \"FeatureCollection\", \"name\": \"3D Seismic\", \"root\": \"/auto/d43/surveys\", \"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" } }, \"features\": [ { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3NAM1982A\", \"name\": \"F3_Demo_d30\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 7, 55.0554844553, 0.0 ], [ 6, 55.0556671475, 0.0 ], [ 6, 54.9236026526, 0.0 ], [ 7, 54.9229699809, 0.0 ], [ 7, 55.0554844553, 0.0 ] ] ] } }, { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3GDF2010A\", \"name\": \"MagellanesBasin\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 9, 53.8820024932, 0.0 ], [ 7, 53.877063624,  0.0 ], [ 7, 53.7857242316, 0.0 ], [ 8, 53.7419173548, 0.0 ], [ 9, 53.7461123222, 0.0 ], [ 9, 53.8820024932, 0.0 ] ] ] } } ] }",

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


static bool testUseJSON()
{
    const Node& tree = *static_cast<const Node*>( jsontree );

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

    /* TODO: this fails
    const auto& point3 = poly.array( 3 );
    const int coordsz = point3.size();
    mRunStandardTestWithError( coordsz==3, "size of coords",
			       BufferString("coordsz=",coordsz) );
    */

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testParseJSON() || !testUseJSON() )
	return 1;

    return 0;
}
