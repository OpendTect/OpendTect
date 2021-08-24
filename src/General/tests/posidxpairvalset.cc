/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/


#include "posidxpairvalset.h"
#include "testprog.h"


static const int cNrObjs = 10;

static bool chckDs( Pos::IdxPairValueSet& ds, const char* msg )
{
    tstStream() << msg << od_endl;
    const int nrvals = ds.nrVals();
    Pos::IdxPairValueSet::SPos spos;
    while ( ds.next(spos) )
    {
	Pos::IdxPair ip = ds.getIdxPair( spos );
	tstStream() << ip.first << '/' << ip.second;
	const float* vals = ds.getVals( spos );
	for ( int idx=0; idx<nrvals; idx++ )
	    tstStream() << '\t' << vals[idx];
	tstStream() << od_endl;
    }

    tstStream() << "All OK." << od_endl;
    return true;
}

static bool checkContents( Pos::IdxPairValueSet& ds )
{

    Pos::IdxPairValueSet::SPos spos;
    Pos::IdxPairValueSet::SPos spostorem;
    Pos::IdxPair ip, iptorem;
    Pos::IdxPairValueSet::IdxType crltorem = 1999 + 3 * cNrObjs / 4;
    while ( ds.next(spos) )
    {
	ip = ds.getIdxPair( spos );
	if ( ip.second == crltorem )
	{
	    spostorem = spos;
	    iptorem = ip;
	}
    }

    chckDs( ds, "** Original:" );
    ds.removeVal( 2 );
    chckDs( ds, "** After remove col 2:" );
    ds.remove( spostorem );
    chckDs( ds, BufferString("** After remove row ",crltorem,":") );
    ds.setNrVals( ds.nrVals() + 1 );

    spos.reset();
    const int lastcol = ds.nrVals() - 1;
    float newval = 1000;
    while ( ds.next(spos) )
    {
	float* vals = ds.getVals( spos );
	vals[lastcol] = newval;
	newval += 1;
    }
    chckDs( ds, BufferString("** After add col ",lastcol,":") );

    return true;
}


static bool testAdd( Pos::IdxPairValueSet& ds, float startval )
{
    tstStream() << "\n\nSet "
		<< (ds.allowsDuplicateIdxPairs() ? "with" : "without")
		<< " duplicates." << od_endl;

    TypeSet<float> vals( ds.nrVals(), 0 );
    int inl = 1001; int crl = 2000;
    for ( int idx=0; idx<cNrObjs; idx++ )
    {
	for ( int ival=0; ival<ds.nrVals(); ival++ )
	    vals[ival] = startval + idx * 100 + ival * 10;
	const float* toadd = idx != 1 ? vals.arr() : 0;
	ds.add( Pos::IdxPair(inl,crl), toadd );
	if ( idx == cNrObjs/2 )
	    inl--;
	else if ( idx != cNrObjs/4 )
	    crl++;
    }

    return checkContents( ds );
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    Pos::IdxPairValueSet ds_nodup( 5, false );
    if ( !testAdd(ds_nodup,1.0f) )
	return 1;
    Pos::IdxPairValueSet ds_dup( 5, true );
    if ( !testAdd(ds_dup,2.0f) )
	return 1;

    return 0;
}
