/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2013
-*/

#include "testprog.h"
#include "seiskeytracker.h"

#define mErrRet(strmstuff) \
{ od_cout() << strmstuff << od_endl; return 1; }

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    const int nrargs = clParser().nrArgs();
    if ( nrargs < 1 )
	mErrRet("Please pass output filename")

    const BufferString fnm( clParser().getArg(0) );
    od_ostream strm( fnm );

    Seis::GeomType geom;
    Seis::KeyTracker* trckr;
#define mStartNewGeomType(gt) \
    geom = Seis::gt; \
    strm << "\n\n** " << Seis::nameOf(geom) << " **\n\n"; \
    Seis::TrackRecord trckrec##gt( geom ); \
    Seis::KeyTracker trckr##gt( trckrec##gt ); \
    trckr = &trckr##gt

#define mDumpTrackRecord() \
    trckr->finish(); \
    trckr->trackRecord().dump( strm, false )

    mStartNewGeomType(Vol);
    trckr->add( BinID(100,311) );
    trckr->add( BinID(100,312) );
    trckr->add( BinID(100,313) );
    trckr->add( BinID(100,315) );
    trckr->add( BinID(100,316) );
    trckr->add( BinID(100,317) );
    trckr->add( BinID(100,319) );
    trckr->add( BinID(102,312) );
    trckr->add( BinID(103,313) );
    trckr->add( BinID(103,314) );
    trckr->add( BinID(103,315) );
    trckr->add( BinID(104,315) );
    trckr->add( BinID(106,315) );
    trckr->add( BinID(108,315) );
    trckr->add( BinID(108,316) );
    trckr->add( BinID(108,317) );
    trckr->add( BinID(108,319) );
    mDumpTrackRecord();

    mStartNewGeomType(VolPS);
    trckr->add( BinID(100,311), 100.f );
    trckr->add( BinID(100,311), 300.f );
    trckr->add( BinID(100,311), 500.f );
    trckr->add( BinID(100,312), 100.f );
    trckr->add( BinID(100,312), 300.f );
    trckr->add( BinID(100,312), 500.f );
    trckr->add( BinID(100,313), 100.f );
    trckr->add( BinID(100,313), 300.f );
    trckr->add( BinID(100,313), 500.f );
    trckr->add( BinID(100,314), 100.f );
    trckr->add( BinID(100,314), 300.f );
    trckr->add( BinID(100,314), 500.f );
    trckr->add( BinID(100,314), 700.f );
    trckr->add( BinID(100,315), 100.f );
    trckr->add( BinID(100,315), 300.f );
    trckr->add( BinID(100,315), 500.f );
    trckr->add( BinID(100,315), 700.f );
    trckr->add( BinID(100,316), 200.f );
    trckr->add( BinID(100,316), 300.f );
    trckr->add( BinID(100,316), 500.f );
    trckr->add( BinID(100,316), 700.f );
    trckr->add( BinID(100,317), 300.f );
    trckr->add( BinID(100,317), 500.f );
    trckr->add( BinID(100,317), 700.f );
    trckr->add( BinID(100,317), 900.f );
    mDumpTrackRecord();

    mStartNewGeomType(Line);
    trckr->add( 311 );
    trckr->add( 312 );
    trckr->add( 313 );
    trckr->add( 315 );
    trckr->add( 317 );
    trckr->add( 319 );
    trckr->add( 323 );
    trckr->add( 325 );
    trckr->add( 327 );
    trckr->add( 328 );
    trckr->add( 329 );
    mDumpTrackRecord();

    mStartNewGeomType(LinePS);
    trckr->add( 311, 100.f );
    trckr->add( 311, 300.f );
    trckr->add( 311, 500.f );
    trckr->add( 312, 100.f );
    trckr->add( 312, 300.f );
    trckr->add( 312, 500.f );
    trckr->add( 313, 100.f );
    trckr->add( 313, 300.f );
    trckr->add( 313, 500.f );
    trckr->add( 314, 100.f );
    trckr->add( 314, 300.f );
    trckr->add( 314, 500.f );
    trckr->add( 314, 700.f );
    trckr->add( 315, 100.f );
    trckr->add( 315, 300.f );
    trckr->add( 315, 500.f );
    trckr->add( 315, 700.f );
    trckr->add( 316, 200.f );
    trckr->add( 316, 300.f );
    trckr->add( 316, 500.f );
    trckr->add( 316, 700.f );
    trckr->add( 317, 300.f );
    trckr->add( 317, 500.f );
    trckr->add( 317, 700.f );
    trckr->add( 317, 900.f );
    mDumpTrackRecord();

    strm.close();
    return 0;
}
