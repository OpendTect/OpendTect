#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id$
________________________________________________________________________

-*/

#include "odsession.h"
#include "uistrings.h"

defineTranslatorGroup(ODSession,"Session setup");
defineTranslator(dgb,ODSession,mDGBKey);

uiString ODSessionTranslatorGroup::sTypeName(int num)
{ return uiStrings::sSession(num); }


