/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "coord.h"
#include "geometry.h"
#include "odjson.h"
#include "stringbuilder.h"
#include "ranges.h"

using namespace OD::JSON;

static ValueSet* jsontree;

static const char* jsonstrs[] = {

// one good
"{ \"type\": \"FeatureCollection\", \"name\": \"3D Seismic\", \"root\": \"/auto/d43/surveys\", \"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" } }, \"features\": [ { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3NAM1982A\", \"name\": \"F3_Demo_d30\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 7, 55.0554844553, 0 ], [ 6, 55.0556671475, 0 ], [ 6, 54.9236026526, 0 ], [ 7, 54.9229699809, 0 ], [ 7, 55.0554844553, 0 ] ] ] } }, { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3GDF2010A\", \"name\": \"MagellanesBasin\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 9, 53.8820024932, 0 ], [ 7, 53.877063624,  0 ], [ 7, 53.7857242316, 0 ], [ 8, 53.7419173548, 0 ], [ 9, 53.7461123222, 0 ], [ 9, 53.8820024932, 0 ] ] ] } } ] }",

// any number of bad ones
"\"aap\": \"noot\"]",
"{\"aap\": \"noot\"",
"{\"aap\": \"noot\"]",
"{ x: \"y\"}",

0
};

static const char* sKeyInterval()	{ return  "Interval"; }


static bool testParseJSON()
{
    BufferStringSet bss( jsonstrs );
    int idx = 0;
    for ( auto strptr : bss )
    {
	BufferString& str = *strptr;
	if ( idx )
	    tstStream() << "\t>> '" << str << "' <<" << od_endl;

	uiRetVal uirv;
	ValueSet* tree = ValueSet::getFromJSon( str.getCStr(), str.size(),uirv);
	const bool isok = uirv.isOK();
	if ( !isok )
	    logStream() << "\tmsg=" << toString(uirv) << od_endl;

	if ( idx )
	    mRunStandardTest( !isok, "Parse bad string" )
	else
	    mRunStandardTest( isok, "JSon parses fine" )

	if ( isok )
	    jsontree = tree;
	idx++;
	if ( idx == 1 )
	    tstStream() << "\nNext strings should not parse:\n\n";
    }

    return true;
}


#define mCheckNonNull( ptr, nm ) \
    mRunStandardTest( ptr != 0, #nm " present" )


static bool testUseJSON( bool created )
{
    const Object& tree = jsontree->asObject();
    tstStream() << "\n\nUse JSON, " << (created ? "Constructed." : "Original.")
		<< od_endl;

    const auto* crsobj = tree.getObject( "crs" );
    mCheckNonNull( crsobj, crs );
    const auto* crspropobj = crsobj->getObject( "properties" );
    mCheckNonNull( crspropobj, crs.properties );
    BufferString namestr = crspropobj->getStringValue( "name" );
    mRunStandardTest( namestr=="urn:ogc:def:crs:OGC:1.3:CRS84", "crs name" );

    const auto* featsarr = tree.getArray( "features" );
    mCheckNonNull( featsarr, features );
    const int sz = featsarr->size();
    mRunStandardTest( sz==2, "Number of features" );
    const auto& feat2 = featsarr->object( 1 );

    const auto* feat2props = feat2.getObject( "properties" );
    mCheckNonNull( feat2props, feats[1].props );
    namestr = feat2props->getStringValue( "name" );
    mRunStandardTest( namestr=="MagellanesBasin", "props.name" );

    const auto* geomobj = feat2.getObject( "geometry" );
    mCheckNonNull( geomobj, feat2.geometry );
    BufferString typestr = geomobj->getStringValue( "type" );
    mRunStandardTest( typestr=="Polygon", "geometry.type" );

    const auto* coords = geomobj->getArray( "coordinates" );
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
    mRunStandardTest( coord==Coord3(8,53.7419173548,0), "coordinate value" );

    return true;
}


static Array* createFeatCoordArray( Array* featarr,
	const char* id, const char* nm, const char* typ )
{
    Object* featobj = featarr->add( new Object );
    featobj->set( "type", "Feature" );

    Object* propobj = featobj->set( "properties", new Object );
    propobj->set( "id", id );
    propobj->set( "name", nm );

    Object* geomobj = featobj->set( "geometry", new Object );
    geomobj->set( "type", typ );

    return geomobj->set( "coordinates", new Array(false) );
}


static void addCoords( const TypeSet<Coord3>& coords, Array& poly )
{
    for ( const auto& coord : coords )
    {
	Array* coordarr = poly.add( new Array(Number) );
	coordarr->add( coord.x ).add( coord.y ).add( coord.z );
    }
}


static bool testCreateJSON()
{
    Object topobj;

    topobj.set( "type", "FeatureCollection" );
    topobj.set( "name", "3D Seismic" );
    topobj.set( "root", "/auto/d43/surveys" );

    Object* crsobj = topobj.set( "crs", new Object );
    crsobj->set( "type", "name" );
    Object* crspropsobj = crsobj->set( "properties", new Object );
    crspropsobj->set( "name", "urn:ogc:def:crs:OGC:1.3:CRS84" );

    Array* featarr = topobj.set( "features", new Array(true) );
    Array* polyarr = createFeatCoordArray( featarr, "Z3NAM1982A", "F3_Demo_d30",
					   "Polygon" );
    Array* poly = polyarr->add( new Array(false) );

    // Once via TypeSet<double> for test
    Array* coord = poly->add( new Array(Number) );
    TypeSet<NumberType> cvals;
    cvals += 7; cvals += 55.0554844553; cvals += 0.0;
    coord->set( cvals );

    // Once via TypeSet for test
    TypeSet<Coord3> coords;
    coords += Coord3( 6, 55.0556671475, 0 );
    coords += Coord3( 6, 54.9236026526, 0 );
    coords += Coord3( 7, 54.9229699809, 0 );
    coords += Coord3( 7, 55.0554844553, 0 );
    addCoords( coords, *poly );

    polyarr = createFeatCoordArray( featarr, "Z3GDF2010A", "MagellanesBasin",
					   "Polygon" );
    poly = polyarr->add( new Array(false) );
    coords.setEmpty();
    coords += Coord3( 9, 53.8820024932, 0 );
    coords += Coord3( 7, 53.877063624,  0 );
    coords += Coord3( 7, 53.7857242316, 0 );
    coords += Coord3( 8, 53.7419173548, 0 );
    coords += Coord3( 9, 53.7461123222, 0 );
    coords += Coord3( 9, 53.8820024932, 0 );
    addCoords( coords, *poly );

    delete jsontree;
    jsontree = topobj.clone();
    return true;
}


static bool testDumpJSON()
{
    StringBuilder sb;
    jsontree->dumpJSon( sb );
    BufferString dumpstr( sb.result() );
	logStream() << "\ndump:\n\n" << dumpstr << '\n' << od_endl;

    BufferString orgstr( jsonstrs[0] );
    dumpstr.remove( ' ' ).remove( '\t' ).remove( '\n' );
    orgstr.remove( ' ' ).remove( '\t' ).remove( '\n' );
    const bool aresame = dumpstr == orgstr;
    if ( !aresame )
	tstStream() << "\norg was:\n\n" << orgstr << '\n' << od_endl;
    mRunStandardTest( aresame, "dumped JSON == original JSON" );

    return true;
}


bool testInterval()
{
    OD::JSON::Object obj;
    const StepInterval<int> intervalwrite( 10, 20, 5 );
    obj.set( sKeyInterval(), intervalwrite );

    StepInterval<int> intervalread;
    obj.get( sKeyInterval(), intervalread );
    mRunStandardTest( intervalread == intervalwrite,
	   "Checking SetInterval And GetInterval" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testParseJSON()
      || !testUseJSON(false)
      || !testCreateJSON()
      || !testUseJSON(true)
      || !testDumpJSON()
      || !testInterval() )
	return 1;

    return 0;
}
