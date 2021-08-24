/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2013
-*/


#include "testprog.h"
#include "seisbuf.h"
#include "seistrc.h"


#define mAddTrc(i,c) \
    trc = new SeisTrc( 10 ); \
    trc->info().binid = BinID(i,c); \
    tbuf.add( trc )

static bool testSorting()
{
    SeisTrcBuf tbuf( true );
    SeisTrc* trc;
    mAddTrc(500,600);
    mAddTrc(499,600);
    mAddTrc(499,599);
    mAddTrc(503,597);
    mAddTrc(502,600);
    mAddTrc(501,599);
    mAddTrc(501,600);
    mAddTrc(502,598);

    tbuf.sortForWrite( false );
    if ( tbuf.get(4)->info().binid != BinID(501,600)
      || tbuf.get(7)->info().binid != BinID(503,597) )
    {
	od_cout() << "Sorting fails." <<  od_endl;
	return false;
    }
    return true;
}

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    return testSorting() ? 0 : 1;
}
