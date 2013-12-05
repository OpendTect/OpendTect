/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "od_iostream.h"
#include "file.h"
#include "filepath.h"
#include "commandlineparser.h"
#include "keystrs.h"
#include "od_iostream.h"

static const BufferString tmpfnm( FilePath::getTempName("txt") );


#define mRetFail(s) { od_ostream::logStream() << "Failed " << s << od_endl; }

#define mImplNumberTestFn(fnnm,decls,act,cond) \
static bool fnnm( bool quiet ) \
{ \
    od_istream strm( tmpfnm ); \
    decls; \
    act; \
    const bool isok = (cond); \
    if ( !quiet ) \
    { \
	od_ostream::logStream() << #fnnm << "[" << #cond << "]" \
    		  << (isok ? " OK" : " Fail") << od_endl; \
	if ( strm.isBad() ) \
		od_ostream::logStream() << "\terrmsg=" << strm.errMsg() << od_endl; \
    } \
    return isok; \
}


mImplNumberTestFn( testIfNumberIsNormal, int i; float f,
	   strm >> i >> f,
	   i == 123 && mIsEqual(f,44.5,1e-8) )

mImplNumberTestFn( testIfNumberIsBad, int i; float f,
	   strm >> i >> f,
	   strm.isBad() )

mImplNumberTestFn( testIfFNumberIs0, int i; float f,
	   strm >> i >> f,
	   i == 123 && f == 0 && strm.isOK() )

mImplNumberTestFn( testOnlyIntRead, int i; float f,
	   strm >> i >> f,
	   i == 123 && !strm.isOK() )


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );
    const bool quiet = CommandLineParser().hasKey( sKey::Quiet() );

    bool isok;
#define mDoTest(strm,content,tstfn) \
    od_ostream strm( tmpfnm ); \
    strm << content; \
    strm.close(); \
    isok = tstfn(quiet); \
    File::remove( tmpfnm ); \
    if ( !isok ) \
	ExitProgram( 1 )

    mDoTest(strm1,"123 44.5",testIfNumberIsNormal);
    mDoTest(strm2,"\t\n123\t\t44.5\n\n",testIfNumberIsNormal);
    mDoTest(strm3,"aap 44.5",testIfNumberIsBad);
    mDoTest(strm4," 123 noot",testIfNumberIsBad);
    mDoTest(strm5,"123\n-0.0e-22\n888",testIfFNumberIs0);
    mDoTest(strm6,"123",testOnlyIntRead);
    mDoTest(strm7,"\n123\n \n",testOnlyIntRead);

    ExitProgram( 0 );
}
