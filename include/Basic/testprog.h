#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Dec 2013
________________________________________________________________________

 Single include getting many of the tools you need for test programs.

 The macro mInitTestProg() will take care of:
 1) Initialisation of program args
 2) A file-scope variable 'bool quiet_': whether progress info is required
 3) A command line parser 'CommandLineParser clParser()'

 When the file is inclueded in a lib source, #define __test_lib_source__. To use
 the macros and inline functions, you'll have to make a variable 'quiet_'
 (e.g. as class member).

-*/

#include "commandlineparser.h"
#include "genc.h"
#include "keystrs.h"
#include "debug.h"
#include "ptrman.h"
#include "od_ostream.h"
#include "odruncontext.h"

#ifndef __test_lib_source__

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
    OD::SetRunContext( OD::TestProgCtxt );
    ExitProgram( testMain( argc, argv ) );
}
# endif

static mUsedVar bool quiet_ = true;
static mUsedVar PtrMan<CommandLineParser> the_testprog_parser_ = 0;

static inline CommandLineParser& clParser()
{
    return *the_testprog_parser_;
}


# define mRunSubTest( nm ) \
    tstStream() << "\n\n\n->" << #nm << " subtest\n\n"; \
    status = test_main_##nm( argc, argv ); \
    if ( status != 0 ) \
	return status

# define mTestProgInits() \
    od_init_test_program( argc, argv ); \
    the_testprog_parser_ = new CommandLineParser; \
    quiet_ = clParser().hasKey( sKey::Quiet() )

# define mInitCompositeTestProg(mod) \
    mTestProgInits(); \
    tstStream() << "** '" << #mod << "' composite test\n\n"; \
    int status

# define mInitTestProg() mTestProgInits()
# define mInitBatchTestProg() \
    int argc = GetArgC(); char** argv = GetArgV(); \
    mInitTestProg()

#endif // ifndef __test_lib_source__


static inline mUsedVar od_ostream& tstStream( bool err=false )
{
    if ( !quiet_ || err )
    {
	if ( err )
	    od_ostream::logStream() << "[FAIL] ";
	return od_ostream::logStream();
    }
    return od_ostream::nullStream();
}

static inline mUsedVar od_ostream& logStream()
{
    return tstStream( false );
}

static inline mUsedVar od_ostream& errStream()
{
    return tstStream( true );
}

inline bool handleTestResult( bool isok, const char* desc, const char* emsg=0 )
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
    { if ( !handleTestResult((test),desc) ) return false; }

#define mRunStandardTestWithError( test, desc, err ) \
    { if ( !handleTestResult((test),desc,err) ) return false; }
