#ifndef testprog_h
#define testprog_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		5-12-1995
 RCS:		$Id$
________________________________________________________________________

 Single include getting many of the tools you need for test programs.

 The macro mInitTestProg() will take care of:
 1) Initialisation of program args
 2) A file-scope variable 'const bool quiet': whether progress info is required
 3) A command line parser 'CommandLineParser clparser'

-*/

#include "commandlineparser.h"
#include "keystrs.h"
#include "debug.h"
#include "od_ostream.h"

static bool quiet = true;

#define mInitTestProg() \
    od_init_test_program( argc, argv ); \
    CommandLineParser clparser; \
    quiet = clparser.hasKey( sKey::Quiet() )

#define mInitBatchTestProg() \
    int argc = GetArgC(); char** argv = GetArgV(); \
    mInitTestProg()


#endif
