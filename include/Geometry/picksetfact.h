#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "picksettr.h"
#include "uistrings.h"

defineTranslatorGroup(PickSet,"PickSet Group")
defineTranslator(dgb,PickSet,mDGBKey)

uiString PickSetTranslatorGroup::sTypeName( int num)
{ return uiStrings::sPointSet( num ); }
