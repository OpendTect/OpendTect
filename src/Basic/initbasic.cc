/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: initbasic.cc,v 1.4 2012-05-22 14:48:29 cvskris Exp $";

#include "moddepmgr.h"

#define OD_EXT_KEYSTR_EXPAND 1

#include "keystrs.h"

mDefModInitFn(Basic)
{
    mIfNotFirstTime( return );
}
