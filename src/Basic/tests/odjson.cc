/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2016
-*/


#include "testprog.h"
#include "odjson.h"

static ObjectSet<OD::JSON::Tree> jsontrees;

static const char* jsonstrs[] = {
"{ aap: noot",
"{ aap: noot }",
"{ "type": "FeatureCollection", "name": "3D Seismic", "root": "/auto/d43/surveys", "crs": { "type": "name", "properties": { "name": "urn:ogc:def:crs:OGC:1.3:CRS84" } }, "features": [ { "type": "Feature", "properties": { "id": "Z3NAM1982A", "name": "F3_Demo_d30" }, "geometry": { "type": "Polygon", "coordinates": [ [ [ 7, 55.0554844553, 0.0 ], [ 6, 55.0556671475, 0.0 ], [ 6, 54.9236026526, 0.0 ], [ 7, 54.9229699809, 0.0 ], [ 7, 55.0554844553, 0.0 ] ] ] } }, { "type": "Feature", "properties": { "id": "Z3GDF2010A", "name": "MagellanesBasin" }, "geometry": { "type": "Polygon", "coordinates": [ [ [ 9, 53.8820024932, 0.0 ], [ 7, 53.877063624,  0.0 ], [ 7, 53.7857242316, 0.0 ], [ 8, 53.7419173548, 0.0 ], [ 9, 53.7461123222, 0.0 ], [ 9, 53.8820024932, 0.0 ] ] ] } } ] }",
0
};


static bool testParseJSON(
{
    BufferStringSet bss( jsonstrs );
    for ( const auto strptr : bss )
    {
	const auto idx = bss.indexOf( str );
	const auto& str = *strptr;

	auto tree = new OD::JSON::Tree;
	uiRetVal uirv;
	tree->parseJSon( str.getCStr(), str.size(), uirv );
	mRunStandardTestWithError( uirv.isOK() = idx != 0, "Parse result" )
	if ( !quiet && idx == 0 )
	    tstStream() << "\tError msg: " << toString(uirv) << od_endl;

	trees += tree;
    }
}


#define mNFStr(typ,nodenm) \
    BufferString( #typ, " '", BufferString(#nodenm, "' not found")


static bool testUseJSON()
{
    for ( const auto treeptr : jsontrees )
    {
	if ( treeptr == jsontrees[0] )
	{
	    const BufferString nootstr = treeptr->getStringValue( "aap" );
	    mRunStandardTestWithError( nootstr == "noot", "aap not noot" )
	    continue;
	}

	const auto& tree = *treeptr;
	const auto* crsnode = tree.getNode( "crs" );
	mRunStandardTestWithError( crsnode, mNFStr(Node,crs) )
	const auto* crspropnode = crsnode->getNode( "properties" );
	mRunStandardTestWithError( crspropnode, mNFStr(Node,crs.properties)" )
	const BufferString namestr = crspropnode->getStringValue( "name" );
	mRunStandardTestWithError( name == "urn:ogc:def:crs:OGC:1.3:CRS84",
				   "crs name" )
	const auto* featsarr = tree.getArray( "features" );
	mRunStandardTestWithError( featsarr, mNFStr(Array,features) )
	const int sz = featsarr->size();
	mRunStandardTestWithError( sz==2, "Number of features" )
        const auto& feat2 = featsarr->getNode( 1 );
        const auto* feat2props = feat2.getNode( "properties" );
	mRunStandardTestWithError( feat2props, mNFStr(Node,feats[1].props) )
        const auto* f2pgeom = feat2props->getNode( "geometry" );
	mRunStandardTestWithError( f2pgeom, mNFStr(Node,props.geom) )
        const Array* coords = f2pgeom->getArray( "coordinates" );
	mRunStandardTestWithError( coords, mNFStr(Node,geom.coords) )
	const Array* point3 = coords->getArray( 3 );
	mRunStandardTestWithError( coords, mNFStr(Node,coords[2]) )
    }
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testParseJSON() || !testUseJSON() )
	return 1;

    return 0;
}
