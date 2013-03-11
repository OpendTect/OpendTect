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
#include "surv2dgeom.h"
#include "errh.h"
#include "filepath.h"

#define OD_EXT_KEYSTR_EXPAND 1

#include "keystrs.h"

mDefModInitFn(Basic)
{
    mIfNotFirstTime( return );
    SignalHandling::initClass();
    PosInfo::Survey2D::initClass();
    System::CrashDumper::getInstance().setSendAppl(
	    				System::CrashDumper::sSenderAppl() );
}
