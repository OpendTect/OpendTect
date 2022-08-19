#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odsession.h"
#include "uistrings.h"

defineTranslatorGroup(ODSession,"Session setup");
defineTranslator(dgb,ODSession,mDGBKey);

uiString ODSessionTranslatorGroup::sTypeName(int num)
{ return uiStrings::sSession(num); }
