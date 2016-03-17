/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2007
-*/


#include "bendpoints2coords.h"
#include "positionlist.h"

#include "testprog.h"
#include "file.h"

#include "od_iostream.h"


#define mTest( testname, test ) \
if ( (test)==true ) \
{ \
    if ( !quiet ) \
	od_ostream::logStream() << testname << ": OK\n"; \
} \
else \
{ \
    od_ostream::logStream() << testname << ": Failed\n"; \
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
    if ( !File::exists( file ) )
    {
	od_ostream::logStream() << "Input file " << file << " does not exist";
	return false;
    }

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

    BufferStringSet normalargs;
    clparser.getNormalArguments(normalargs);

    if ( !testCoordList2D() )
	ExitProgram( 1 );
    if ( !testCoordList3D() )
	ExitProgram( 1 );

    if ( normalargs.isEmpty() )
    {
	od_ostream::logStream() << "No input file specified";
	ExitProgram( 1 );
    }

    if ( !testReadBendPointFile( normalargs.get(0) ) )
	ExitProgram( 1 );

    ExitProgram( 0 );
}
