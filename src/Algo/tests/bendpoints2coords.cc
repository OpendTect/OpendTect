/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bendpoints2coords.h"
#include "positionlist.h"

#include "testprog.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "sorting.h"

#include "od_iostream.h"


#define mTest( testname, test ) \
if ( (test)==true ) \
{ \
    handleTestResult( true, testname ); \
} \
else \
{ \
    handleTestResult( false, testname ); \
    return false; \
}


static bool testCoordList2D()
{
    Coord2ListImpl clist;
    clist.add( Coord(100,100) );
    clist.add( Coord(200,300) );
    const Coord center( clist.center() );
    return mIsZero(center.x-150,0.01) && mIsZero(center.y-200,0.01);
}

static bool testCoordList3D()
{
    Coord3ListImpl clist;
    clist.add( Coord3(100,100,500) );
    clist.add( Coord3(200,300,700) );
    const Coord3 center( clist.center() );
    return mIsZero(center.x-150,0.01) && mIsZero(center.y-200,0.01)
	&& mIsZero(center.z-600,0.01);
}


static bool testReadBendPointFile( const char* file )
{
    od_istream stream( file );
    if ( !stream.isOK() )
    {
	od_ostream::logStream() << "Could not open " << file;
	return false;
    }

    BendPoints2Coords b2dcrd( stream );

    mTest( "Bendpoint reading nr crds", b2dcrd.getCoords().size()==2 );
    mTest( "Bendpoint reading nr ids", b2dcrd.getIDs().size()==2 );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    FilePath fp( __FILE__ );
    fp.setExtension( "txt" );
    if ( !fp.exists() )
    {
	fp.set( GetSoftwareDir(false) ).add( __FILE__ ).setExtension( "txt" );
	if ( !fp.exists() )
	{
	    errStream() << "Input file not found\n";
	    return 1;
	}
    }

    if ( !testCoordList2D() )
	return 1;
    if ( !testCoordList3D() )
	return 1;

    const BufferString parfile( fp.fullPath() );
    if ( !testReadBendPointFile(parfile) )
	return 1;

    return 0;
}
