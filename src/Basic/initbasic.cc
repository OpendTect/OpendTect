/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "debug.h"
#include "sighndl.h"
#include "posinfo2dsurv.h"
#include "filepath.h"
#ifdef __win__
# include <stdio.h> // for _set_output_format
#endif

#define OD_EXT_KEYSTR_EXPAND 1

#include "keystrs.h"



mDefModInitFn(Basic)
{
    mIfNotFirstTime( return );

    SignalHandling::initClass();

#ifdef __win__
    _set_output_format(_TWO_DIGIT_EXPONENT);
    // From MSDN:
    // "is used to configure the output of formatted I/O functions"
#endif

    PosInfo::Survey2D::initClass();

#ifdef mUseCrashDumper
    System::CrashDumper::getInstance().setSendAppl(
					System::CrashDumper::sSenderAppl() );
#endif

}
