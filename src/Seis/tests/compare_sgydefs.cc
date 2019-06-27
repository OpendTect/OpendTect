/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2013
-*/

#include "testprog.h"
#include "segydirectdef.h"
#include "segyfiledata.h"
#include "posinfo.h"


#define mErrRet(strmstuff) { od_cout() << strmstuff << od_endl; return false; }

static bool compareDefs( const SEGY::DirectDef& dd1, const SEGY::DirectDef& dd2)
{
    const PosInfo::CubeData& cd1 = dd1.cubeData();
    const PosInfo::CubeData& cd2 = dd2.cubeData();
    const int cdsz = cd1.size();
    if ( cdsz != cd2.size() )
	mErrRet("CubeData sizes differ")

    for ( int iln=0; iln<cdsz; iln++ )
    {
	if ( *cd1[iln] != *cd2[iln] )
	    mErrRet("LineData for "<<iln<<"th line differs")
    }

    const SEGY::FileDataSet& fds1 = dd1.fileDataSet();
    const SEGY::FileDataSet& fds2 = dd2.fileDataSet();
    const od_int64 fdssz = fds1.size();
    if ( fdssz != fds2.size() )
	mErrRet("FileDataSet sizes differ")

    const int nrfiles = fds1.nrFiles();
    if ( nrfiles != fds2.nrFiles() )
	mErrRet("Number of files differ")
    for ( od_int64 ifile=0; ifile<nrfiles; ifile++ )
    {
	const BufferString fnm( fds1.fileName(ifile) );
	if ( fnm != fds1.fileName(ifile) )
	    mErrRet("Files name "<<ifile<<" "<<fnm)
    }

    Seis::PosKey pk1, pk2;
    bool usable1, usable2;
    for ( od_int64 itrc=0; itrc<fdssz; itrc++ )
    {
	bool canget1 = fds1.getDetails( itrc, pk1, usable1 );
	bool canget2 = fds2.getDetails( itrc, pk2, usable2 );
	if ( canget1 != canget2 )
	    mErrRet("Cannot get details from " << (canget1 ? "2" : "1"))
	if ( !canget1 )
	    continue;

	if ( pk1 != pk2 )
	    mErrRet("PosKey differs at trace " << itrc)
	if ( usable1 != usable1 )
	    mErrRet("usable differs at trace " << itrc)

	SEGY::FileDataSet::TrcIdx tidx = fds1.getFileIndex( itrc );
	if ( tidx != fds2.getFileIndex(itrc) )
	    mErrRet("TrcIdx differs at trace " << itrc)
    }

    od_cout() << "All OK" << od_endl;
    return true;
}


#undef mErrRet
#define mErrRet(strmstuff) \
{ od_cout() << strmstuff << od_endl; return false; }


static bool writeDefToFile( SEGY::DirectDef& dd, const char* fnm )
{
    od_cout() << "Writing " << fnm << " ..." << od_endl;
    if ( !dd.writeHeadersToFile(fnm) )
	mErrRet("Cannot write headers for "<<fnm)
    dd.fileDataSet().dump( *dd.getOutputStream() );
    if ( !dd.writeFootersToFile() )
	mErrRet("Cannot write footers for "<<fnm)

    return true;
}


#undef mErrRet
#define mErrRet(strmstuff) \
{ od_cout() << strmstuff << od_endl; return 1; }


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( clParser().nrArgs() < 2 )
	mErrRet("Please pass two full .sgydef file names")

    od_cout() << "Reading " << clParser().getArg(0) << " ..." << od_endl;
    SEGY::DirectDef dd1( clParser().getArg(0) );
    if ( !dd1.errMsg().isEmpty() )
	mErrRet( toString(dd1.errMsg()) )
    od_cout() << "Reading " << clParser().getArg(1) << " ..." << od_endl;
    SEGY::DirectDef dd2( clParser().getArg(1) );
    if ( !dd2.errMsg().isEmpty() )
	mErrRet( toString(dd2.errMsg()) )

    if ( !compareDefs(dd1,dd2) )
	return 1;

    if ( !writeDefToFile( dd1, "/tmp/dd1.txt" ) )
	return 1;

    if ( !writeDefToFile( dd2, "/tmp/dd2.txt" ) )
	return 1;

    return 0;
}
