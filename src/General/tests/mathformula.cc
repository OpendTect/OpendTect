/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "mathformula.h"
#include "testprog.h"


#define mTestFail(var) \
    { od_cout() << "Fail:\n" << #var << '=' << var << od_endl; return false; }

static bool testSimpleFormula()
{
     const char* expr = "c0 * x + y";

    if ( !quiet )
	od_cout() << "Expression:\n" << expr << "\n\n";

    Math::Formula form( expr );

    if ( !form.isOK() )
	{ od_cout() << "Fail:\n" << form.errMsg() << od_endl; return false; }

    const int nrinp = form.nrInputs();
    if ( nrinp != 3 )
	mTestFail(nrinp)

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testSimpleFormula() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
