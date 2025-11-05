#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 Single include getting many of the tools you need for test programs.

 The macro mInitTestProg() will take care of:
 1) Initialization of program args
 2) Setting OD::GetQuietFlag(): whether progress info is required
 3) A command line parser 'CommandLineParser& clParser()'

-*/

#include "applicationdata.h"
#include "commandlineparser.h"
#include "genc.h"
#include "keystrs.h"
#include "debug.h"
#include "moddepmgr.h"
#include "ptrman.h"
#include "od_ostream.h"
#include "odruncontext.h"

#ifdef __testprog__
# ifndef __prog__

# ifdef __win__
#  include "winmain.h"
# endif

// Use this in stand-alone test
# define mTestMainFnName testMain

int testMain(int,char**);

# ifndef mMainIsDefined
# define mMainIsDefined
int main(int argc, char** argv)
{
    OD::SetRunContext( OD::RunCtxt::TestProgCtxt );
    ExitProgram( testMain(argc,argv) );
}
# endif

# endif // ifndef __prog__

static PtrMan<CommandLineParser> the_testprog_parser_ mUnusedVar = nullptr;

static inline CommandLineParser& clParser()
{
    return *the_testprog_parser_;
}

# define mTestProgInits( withdataroot ) \
    od_init_test_program( argc, argv, withdataroot ); \
    OD::SetQuietFlag( argc, argv ); \
    ApplicationData::sSetDefaults(); \
    the_testprog_parser_ = new CommandLineParser;

# define mInitTestProg() \
    mTestProgInits( false ) \
    OD::ModDeps().ensureLoaded( "Basic" );

# define mInitTestProgDR() \
    mTestProgInits( true ) \
    OD::ModDeps().ensureLoaded( "Basic" );


#endif // ifdef __testprog__


static inline mUnusedVar od_ostream& tstStream( bool err=false,
						bool withprefix=false )
{
    if ( !OD::GetQuietFlag() || err )
    {
	if ( err && withprefix )
	    od_ostream::logStream() << "[FAIL] ";
	return od_ostream::logStream();
    }
    return od_ostream::nullStream();
}

static inline mUnusedVar od_ostream& logStream()
{
    return tstStream( false );
}

static inline mUnusedVar od_ostream& errStream( bool useprefix=true )
{
    return tstStream( true, useprefix );
}

inline bool handleTestResult( bool isok, const char* desc,
			      const char* emsg=nullptr )
{
    if ( isok )
	logStream() << "[OK] " << desc << od_endl;
    else
    {
	if ( !emsg )
	    emsg = "<no details>";
	errStream() << desc << ": " << emsg << od_endl;
    }

    return isok;
}

#define mRunStandardTest( test, desc ) \
    { const bool testres = test; \
	if ( !handleTestResult(testres,desc) ) return false; }

#define mRunStandardTestWithError( test, desc, err ) \
    { const bool testres = test ; \
      if ( !handleTestResult(testres,desc,err) ) return false; }
