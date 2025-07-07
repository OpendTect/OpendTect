/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "arrayndimpl.h"
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
        coordarr->add( coord.x_ ).add( coord.y_ ).add( coord.z_ );
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


bool testArray1D()
{
    OD::JSON::Object jsobj_out;
    const int nrows = 4;
    Array1DImpl<int> arrout_int( nrows );
    for ( int idx=0; idx<arrout_int.size(); idx++ )
	arrout_int.set( idx, idx*2 );

    jsobj_out.set( "array1d_int", arrout_int );

    Array1DImpl<double> arrout_double( nrows );
    for ( int idx=0; idx<arrout_double.size(); idx++ )
	arrout_double.set( idx, idx*2 );

    jsobj_out.set( "array1d_double", arrout_double );

    BufferString jsStr = jsobj_out.dumpJSon();

    logStream() << "\ndump 1D array json:\n" << jsStr << '\n' << od_endl;

    BufferString teststr( "[0,2,4,6]" );
    OD::JSON::Array jsarr_in( false );
    uiRetVal uirv = jsarr_in.parseJSon( teststr.getCStr(), teststr.size() );
    mRunStandardTestWithError( uirv.isOK(),
			      "Parse an array string into a JSON::Array",
			      uirv.getText() );

    OD::JSON::Object jsobj_in;
    teststr.set( "[0,2,4,6]" );
    uirv = jsobj_in.parseJSon( teststr.getCStr(), teststr.size() );
    mRunStandardTestWithError( !uirv.isOK(),
			       "An array should not parse into a JSON::Object",
			       uirv.getText() );

    uirv = jsobj_in.parseJSon( jsStr.getCStr(), jsStr.size() );
    mRunStandardTestWithError( uirv.isOK(),
			       "Parse an object string into a JSON::Object",
			       uirv.getText() );

    jsStr = jsobj_out.dumpJSon();
    OD::JSON::Array jsarr_in2( false );
    uirv = jsarr_in2.parseJSon( jsStr.getCStr(), jsStr.size() );
    mRunStandardTestWithError( !uirv.isOK(),
			       "An object should not parse into a JSON::Array",
			       uirv.getText() );

    Array1DImpl<int> arrin_int(1);
    if ( jsobj_in.get("array1d_int", arrin_int) )
    {
	const int sz = arrin_int.size();
	mRunStandardTestWithError( sz==arrout_int.size(), "Array1Dint size",
				   BufferString("sz=",sz) );
	BufferString str;
	bool allequal = true;
	for ( int idx=0; idx<sz; idx++ )
	{
	    if ( arrin_int[idx]!=arrout_int[idx] )
	    {
		allequal = false;
		str.add("idx: ").add(idx).add(" arrin_int: ")
		    .add(arrin_int[idx]).add(" arrout_int: ")
		    .add(arrout_int[idx]).addNewLine();
	    }
	}
	mRunStandardTestWithError( allequal, "Array1Dint values", str );
    }
    else
	mRunStandardTestWithError( false, "Array1Dint values",
				   "not recovered" );


    Array1DImpl<double> arrin_double(1);
    if ( jsobj_in.get("array1d_double", arrin_double) )
    {
	const int sz = arrin_double.size();
	mRunStandardTestWithError( sz==arrout_double.size(),
				   "Array1Ddouble size",
				   BufferString("sz=",sz) );
	BufferString str;
	bool allequal = true;
	for ( int idx=0; idx<sz; idx++ )
	{
	    if ( arrin_double[idx]!=arrout_double[idx] )
	    {
		allequal = false;
		str.add("idx: ").add(idx).add(" arrin_double: ")
		    .add(arrin_double[idx]).add(" arrout_double: ")
		    .add(arrout_double[idx]).addNewLine();
	    }
	}
	mRunStandardTestWithError( allequal, "Array1Ddouble values", str );
    }
    else
	mRunStandardTestWithError( false, "Array1Ddouble values",
				   "not recovered" );

    return true;
}


bool testArray2D()
{
    OD::JSON::Object jsobj_out;
    const int nrows = 4;
    const int ncols = 5;
    Array2DImpl<int> arrout_int( nrows, ncols );
    for ( int idx=0; idx<arrout_int.totalSize(); idx++ )
	arrout_int.getData()[idx] = idx*2;

    jsobj_out.set( "array2d_int", arrout_int );

    Array2DImpl<double> arrout_double( nrows, ncols );
    for ( int idx=0; idx<arrout_double.totalSize(); idx++ )
	arrout_double.getData()[idx] = idx*2;

    jsobj_out.set( "array2d_double", arrout_double );

    BufferString jsStr = jsobj_out.dumpJSon();

    logStream() << "\ndump 2D array:\n\n" << jsStr << '\n' << od_endl;

    OD::JSON::Object jsobj_in;
    const uiRetVal uirv = jsobj_in.parseJSon( jsStr.getCStr(), jsStr.size() );
    mRunStandardTestWithError( uirv.isOK(),
			       "Parsed an object string into a JSON::Object",
			       uirv.getText() );


    Array2DImpl<int> arrin_int(1,1);
    if ( jsobj_in.get("array2d_int", arrin_int) )
    {
	const int szrows = arrin_int.getSize(0);
	const int szcols = arrin_int.getSize(1);
	BufferString str( "sz= ", szrows, " " );
	str.add(szcols);
	mRunStandardTestWithError( szrows==nrows&&szcols==ncols,
				   "Array2Dint size", str );
	str.setEmpty();
	bool allequal = true;
	for ( int irow=0; irow<szrows; irow++ )
	{
	    for ( int icol=0; icol<szcols; icol++ )
	    {
		if ( arrin_int.get(irow,icol)!=arrout_int.get(irow,icol) )
		{
		    allequal = false;
		    str.add("row: ").add(irow).add(" col: ").add(icol)
			.add(" arrin_int: ").add(arrin_int.get(irow,icol))
			.add(" arrout_int: ").add(arrout_int.get(irow,icol))
			.addNewLine();
		}
	    }
	}
	mRunStandardTestWithError( allequal, "Array2Dint values", str );
    }
    else
	mRunStandardTestWithError( false, "Array2Dint values",
				   "not recovered" );

    Array2DImpl<double> arrin_double(1,1);
    if ( jsobj_in.get("array2d_double", arrin_double) )
    {
	const int szrows = arrin_double.getSize(0);
	const int szcols = arrin_double.getSize(1);
	BufferString str( "sz= ", szrows, " " );
	str.add(szcols);
	mRunStandardTestWithError( szrows==nrows&&szcols==ncols,
				   "Array2Ddouble size", str );
	str.setEmpty();
	bool allequal = true;
	for ( int irow=0; irow<szrows; irow++ )
	{
	    for ( int icol=0; icol<szcols; icol++ )
	    {
		if ( arrin_double.get(irow,icol)!=arrout_double.get(irow,icol) )
		{
		    allequal = false;
		    str.add("row: ").add(irow).add(" col: ").add(icol)
			.add(" arrin_double: ").add(arrin_double.get(irow,icol))
			.add(" arrout_double: ")
			.add(arrout_double.get(irow,icol)).addNewLine();
		}
	    }
	}
	mRunStandardTestWithError( allequal, "Array2Ddouble values", str );
    }
    else
	mRunStandardTestWithError( false, "Array2Ddouble values",
				   "not recovered" );

    return true;
}


bool testMixedArray()
{
    {
	Array jsarr( Mixed );
	jsarr.add("Added String").add(4).add(3.56).add(0==1);
	const bool allequal = jsarr.getStringValue(0)=="Added String" &&
			      jsarr.getIntValue(1)==4 &&
			      jsarr.getDoubleValue(2)==3.56 &&
			      jsarr.getBoolValue(3)==false;
	mRunStandardTest(allequal, "Add values to mixed value type array OK");
    }

    {
	Array jsarr( Mixed );
	BufferString inp("[9,true,\"String\",-3.142]");
	const uiRetVal uirv =
		jsarr.parseJSon( BufferString(inp).getCStr(), inp.size() );
	mRunStandardTestWithError( uirv.isOK(),
		       "Parsed a mixed array string into a mixed JSON::Array",
		       uirv.getText());

	const bool allequal = jsarr.getIntValue(0)==9 &&
			      jsarr.getBoolValue(1)==true &&
			      jsarr.getStringValue(2)=="String" &&
			      jsarr.getDoubleValue(3)==-3.142;
	mRunStandardTest(allequal,
			 "Parse mixed value type array from string OK");
	BufferString jsStr = jsarr.dumpJSon();
	mRunStandardTest(inp==jsStr,
			 "Mixed value type array - dumped JSON == input JSON");
    }
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
      || !testInterval()
      || !testArray1D()
      || !testArray2D()
      || !testMixedArray()
    )
	return 1;

    return 0;
}
