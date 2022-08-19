#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistrings.h"

defineTranslatorGroup(RandomLineSet,"RandomLine Geometry");
defineTranslator(dgb,RandomLineSet,mDGBKey);

uiString RandomLineSetTranslatorGroup::sTypeName( int num )
{ return uiStrings::sRandomLine( num ); }
