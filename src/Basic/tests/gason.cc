/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"
#include "../gason.h"

static const char* jsonstr =

// "{ \"ky1\": { \"ky1.1\": \"1.1\", \"ky1.2\": \"1.2\" } }";

"{ \"type\": \"FeatureCollection\", \"name\": \"3D Seismic\", \"root\": \"/auto/d43/surveys\", \"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" } }, \"features\": [ { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3NAM1982A\", \"name\": \"F3_Demo_d30\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 7, 55.0554844553, 0.0 ], [ 6, 55.0556671475, 0.0 ], [ 6, 54.9236026526, 0.0 ], [ 7, 54.9229699809, 0.0 ], [ 7, 55.0554844553, 0.0 ] ] ] } }, { \"type\": \"Feature\", \"properties\": { \"id\": \"Z3GDF2010A\", \"name\": \"MagellanesBasin\" }, \"geometry\": { \"type\": \"Polygon\", \"coordinates\": [ [ [ 9, 53.8820024932, 0.0 ], [ 7, 53.877063624,  0.0 ], [ 7, 53.7857242316, 0.0 ], [ 8, 53.7419173548, 0.0 ], [ 9, 53.7461123222, 0.0 ], [ 9, 53.8820024932, 0.0 ] ] ] } } ] }";


static BufferString workstr( jsonstr );


static void printGasonValue( od_ostream& strm, const Gason::JsonValue& gsonval )
{
    const Gason::JsonTag tag = gsonval.getTag();
    switch ( tag )
    {
	case Gason::JSON_NUMBER:
	{
	    strm << gsonval.toNumber() << ",";
	}
	break;
	case Gason::JSON_STRING:
	{
	    strm << '"' << gsonval.toString() << "\",";
	}
	break;
	case Gason::JSON_ARRAY:
	{
	    strm << " [ ";
	    for ( auto node : gsonval )
		printGasonValue( strm, node->value );
	    strm << " ]";
	}
	break;
	case Gason::JSON_OBJECT:
	{
	    strm << " { ";
	    for ( auto node : gsonval )
	    {
		strm << "'" << (node->key?node->key:"No Key!") << "': ";
		printGasonValue( strm, node->value );
	    }
	    strm << " }";
	}
	break;
	case Gason::JSON_TRUE:
	    strm << "true,";
	break;
	case Gason::JSON_FALSE:
	    strm << "false,";
	break;
	case Gason::JSON_NULL:
	    strm << "null,";
	break;
    }
    strm << od_endl;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    Gason::JsonValue root;
    Gason::JsonAllocator allocator;
    char *endptr;
    int status = Gason::jsonParse( workstr.getCStr(), &endptr, &root,
		 allocator );
    if ( status != Gason::JSON_OK )
    {
	od_ostream& strm = tstStream( true );
	strm << "Status: " << Gason::jsonStrError(status) << od_endl;
	strm << "at: " << endptr << od_endl;
	return 1;
    }

    if ( !quiet_ )
	printGasonValue( tstStream(false), root );

    return 0;
}
