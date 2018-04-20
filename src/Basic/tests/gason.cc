/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2016
-*/


#include "testprog.h"
#include "../gason.h"

static const char* jsonstr =
"{ \"type\": \"FeatureCollection\", \"name\": \"3D Seismic\", \"root\": \"/auto/d43/surveys\", \"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" } }, \"features\": [ { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3NAM1982A\", \"name\": \"F3_Demo_d30\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 7, 55.0554844553, 0.0 ], [ 6, 55.0556671475, 0.0 ], [ 6, 54.9236026526, 0.0 ], [ 7, 54.9229699809, 0.0 ], [ 7, 55.0554844553, 0.0 ] ] ] } }, { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3GDF2010A\", \"name\": \"MagellanesBasin\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 9, 53.8820024932, 0.0 ], [ 7, 53.877063624,  0.0 ], [ 7, 53.7857242316, 0.0 ], [ 8, 53.7419173548, 0.0 ], [ 9, 53.7461123222, 0.0 ], [ 9, 53.8820024932, 0.0 ] ] ] } } ] }";
static BufferString workstr( jsonstr );


static void printGasonValue( od_ostream& strm, const Gason::JsonValue o )
{
    switch ( o.getTag() )
    {
	case Gason::JSON_NUMBER:
	    strm << o.toNumber();
	break;
	case Gason::JSON_STRING:
	    strm << o.toString();
	break;
	case Gason::JSON_ARRAY:
	for ( auto i : o )
	    printGasonValue( strm, i->value );
	break;
	case Gason::JSON_OBJECT:
	    for ( auto i : o )
	    {
		strm << i->key;
		printGasonValue( strm, i->value );
	    }
	break;
	case Gason::JSON_TRUE:
	    strm << "true";
	break;
	case Gason::JSON_FALSE:
	    strm << "false";
	break;
	case Gason::JSON_NULL:
	    strm << "null" << od_endl;
	break;
    }
}


static bool testGasonParse( Gason::JsonValue& value )
{
    char *endptr;
    Gason::JsonAllocator allocator;
    int status = Gason::jsonParse( workstr.getCStr(), &endptr, &value,
		 allocator );
    if ( status != Gason::JSON_OK )
    {
	od_ostream& strm = tstStream( true );
	strm << "Status: " << Gason::jsonStrError(status) << od_endl;
	strm << "at: " << endptr << od_endl;
	return false;
    }

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    Gason::JsonValue value;
    if ( !testGasonParse(value) )
	return 1;

    printGasonValue( tstStream(false), value );

    return 0;
}
