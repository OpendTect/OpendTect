/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Wayne Mogg
 * DATE     : Aug 2021
 * FUNCTION :
-*/


#include "thread.h"
#include "testprog.h"
#include "math2.h"
#include "genc.h"
#include "mnemonics.h"


bool test_ranges()
{
    const Mnemonic* mnem = MNC().find( "DT" );
    const Mnemonic::DispDefs& disp = mnem->disp_;

    mRunStandardTest(disp.typicalrange_==Interval<float>(200,50),
							     "typicalrange_");
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !test_ranges() )
	return 1;

    return 0;
}
