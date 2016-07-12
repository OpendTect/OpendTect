#ifndef testprog_h
#define testprog_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2013
________________________________________________________________________

 Single include getting many of the tools you need for test programs.

 The macro mInitTestProg() will take care of:
 1) Initialisation of program args
 2) A file-scope variable 'bool quiet': whether progress info is required
 3) A command line parser 'CommandLineParser clparser'

-*/

#include "commandlineparser.h"
#include "keystrs.h"
#include "debug.h"
#include "ptrman.h"
#include "od_ostream.h"

# ifdef __win__
#  include "winmain.h"
# endif

int testMain( int argc, char** argv );

#ifndef batchprog_h
int main(int argc, char** argv)
{
    ExitProgram( testMain( argc, argv ) );
}
#endif

static mUsedVar bool quiet = true;
static mUsedVar PtrMan<CommandLineParser> theparser = 0;

static inline mUsedVar od_ostream& tstStream( bool err=false )
{
    if ( !quiet || err )
    {
	if ( err )
	    od_ostream::logStream() << "[FAIL] ";
	return od_ostream::logStream();
    }
    return od_ostream::nullStream();
}


#define mInitTestProg() \
    od_init_test_program( argc, argv ); \
    theparser = new CommandLineParser; \
    CommandLineParser& clparser = *theparser; \
    quiet = clparser.hasKey( sKey::Quiet() )

#define mExitTestProg( var )

#define mInitBatchTestProg() \
    int argc = GetArgC(); char** argv = GetArgV(); \
    mInitTestProg()

#define mRunStandardTestWithError( test, desc, err ) \
if ( !quiet ) \
    od_ostream::logStream() << desc;  \
if ( (test) ) \
{ \
    if ( !quiet ) \
	od_ostream::logStream() << " - SUCCESS\n"; \
} \
else \
{ \
    if ( quiet ) \
	od_ostream::logStream() << desc; \
    od_ostream::logStream() << " - FAIL"; \
    if ( err ) \
	od_ostream::logStream() << ": " << err << "\n"; \
\
    return false; \
}

#define mRunStandardTest( test, desc ) \
	mRunStandardTestWithError( test, desc, BufferString().str() )


#endif
