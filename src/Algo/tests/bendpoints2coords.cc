/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "bendpoints2coords.h"
#include "positionlist.h"

#include "testprog.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"

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


int main( int argc, char** argv )
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
	    ExitProgram( 1 );
	}
    }

    if ( !testCoordList2D() )
	ExitProgram( 1 );
    if ( !testCoordList3D() )
	ExitProgram( 1 );

    const BufferString parfile( fp.fullPath() );
    if ( !testReadBendPointFile(parfile) )
	ExitProgram( 1 );

    ExitProgram( 0 );
}
