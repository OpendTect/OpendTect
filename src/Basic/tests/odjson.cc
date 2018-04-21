/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2016
-*/


#include "testprog.h"
#include "odjson.h"

using namespace OD::JSON;

static ObjectSet<ValueSet> jsontrees;

static const char* jsonstrs[] = {
"{ \"aap\":::: \"noot\"}",
"{ \"aap\": \"noot\" }",
"{ \"type\": \"FeatureCollection\", \"name\": \"3D Seismic\", \"root\": \"/auto/d43/surveys\", \"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" } }, \"features\": [ { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3NAM1982A\", \"name\": \"F3_Demo_d30\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 7, 55.0554844553, 0.0 ], [ 6, 55.0556671475, 0.0 ], [ 6, 54.9236026526, 0.0 ], [ 7, 54.9229699809, 0.0 ], [ 7, 55.0554844553, 0.0 ] ] ] } }, { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3GDF2010A\", \"name\": \"MagellanesBasin\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 9, 53.8820024932, 0.0 ], [ 7, 53.877063624,  0.0 ], [ 7, 53.7857242316, 0.0 ], [ 8, 53.7419173548, 0.0 ], [ 9, 53.7461123222, 0.0 ], [ 9, 53.8820024932, 0.0 ] ] ] } } ] }",
0
};


static bool testParseJSON()
{
    BufferStringSet bss( jsonstrs );
    for ( const auto strptr : bss )
    {
	const auto idx = bss.indexOf( strptr );
	BufferString str = *strptr;

	uiRetVal uirv;
	ValueSet* tree = ValueSet::parseJSon( str.getCStr(), str.size(), uirv );
	mRunStandardTest( uirv.isOK() == (idx != 0), "Parse result" )
	if ( !quiet && idx == 0 )
	    tstStream() << "\tError msg: " << toString(uirv) << od_endl;

	if ( tree )
	    jsontrees += tree;
    }

    return true;
}


#define mNFStr(typ,nodenm) \
    BufferString( #typ, " '", BufferString(#nodenm, "' in data" )
#define mCheckNonNull( ptr, nm ) \
    mRunStandardTest( ptr != 0, #nm " present" )


static bool testUseJSON()
{
/*
    for ( const auto treeptr : jsontrees )
    {
	const auto& tree = *static_cast<const Node*>( treeptr );
	if ( treeptr == jsontrees[0] )
	{
	    const BufferString nootstr = tree.getStringValue( "aap" );
	    mRunStandardTest( nootstr == "noot", "aap being noot" )
	    continue;
	}

	const auto* crsnode = tree.getNode( "crs" );
	mCheckNonNull( crsnode, crs );
	const auto* crspropnode = crsnode->getNode( "properties" );
	mCheckNonNull( crspropnode, crs.properties );
	const BufferString namestr = crspropnode->getStringValue( "name" );
	mRunStandardTest( namestr == "urn:ogc:def:crs:OGC:1.3:CRS84",
				   "crs name" );
	const auto* featsarr = tree.getArray( "features" );
	mCheckNonNull( featsarr, features );
	const int sz = featsarr->size();
	mRunStandardTest( sz==2, "Number of features" );
        const auto& feat2 = featsarr->node( 1 );
        const auto* feat2props = feat2.getNode( "properties" );
	mCheckNonNull( feat2props, feats[1].props );
        const auto* f2pgeom = feat2props->getNode( "geometry" );
	mCheckNonNull( f2pgeom, props.geom );
        const Array* coords = f2pgeom->getArray( "coordinates" );
	mCheckNonNull( coords, geom.coords );
	const int nrcoords = coords->size();
	mRunStandardTest( nrcoords==6, "Number of coords" );
	const Array& point3 = coords->array( 3 );
	mRunStandardTest( point3.size()==3, "size of coords");
    }
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
