#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2013
 RCS:		$Id$
________________________________________________________________________

 Single include getting many of the tools you need for test programs.

 The macro mInitTestProg() will take care of:
 1) Initialisation of program args
 2) A file-scope variable 'bool quiet_': whether progress info is required
 3) A command line parser 'CommandLineParser& clParser()'

-*/

#include "commandlineparser.h"
#include "genc.h"
#include "keystrs.h"
#include "debug.h"
#include "ptrman.h"
#include "od_ostream.h"
#include "odruncontext.h"

# ifdef __win__
#  include "winmain.h"
# endif

// Kept for ABI, do not use
static mUsedVar bool quiet = true;
static mUsedVar PtrMan<CommandLineParser> theparser = 0;
//

static bool quiet_ mUnusedVar = true;
static PtrMan<CommandLineParser> the_testprog_parser_ mUnusedVar = nullptr;

static inline CommandLineParser& clParser()
{
    return *the_testprog_parser_;
}


# define mRunSubTest( nm ) \
    tstStream() << "\n\n\n->" << #nm << " subtest\n\n"; \
    status = test_main_##nm( argc, argv ); \
    if ( status != 0 ) \
        return status

#define mTestProgInits() \
    od_init_test_program( argc, argv ); \
    the_testprog_parser_ = new CommandLineParser; \
    quiet_ = clParser().hasKey( sKey::Quiet() ); \
    CommandLineParser& parser mUnusedVar = *the_testprog_parser_; \
    quiet = quiet_;

# define mInitCompositeTestProg(mod) \
    mTestProgInits(); \
    tstStream() << "** '" << #mod << "' composite test\n\n"; \
    int status

#define mExitTestProg( var )

# define mInitTestProg() mTestProgInits()
# define mInitBatchTestProg() \
    OD::SetRunContext( OD::TestProgCtxt ); \
    int argc = GetArgC(); char** argv = GetArgV(); \
    mInitTestProg()


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

static inline mUnusedVar od_ostream& logStream()
{
    return tstStream( false );
}

static inline mUnusedVar od_ostream& errStream()
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
    { const bool testres = test; \
	if ( !handleTestResult(testres,desc) ) return false; }

#define mRunStandardTestWithError( test, desc, err ) \
    { const bool testres = test ; \
      if ( !handleTestResult(testres,desc,err) ) return false; }
