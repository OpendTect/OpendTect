#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribdescsettr.h"
#include "uistrings.h"

defineTranslatorGroup(AttribDescSet,"Attribute definitions");
defineTranslator(dgb,AttribDescSet,mDGBKey);

mDefSimpleTranslatorSelector(AttribDescSet);
mDefSimpleTranslatorioContext(AttribDescSet, Attr );

uiString AttribDescSetTranslatorGroup::sTypeName(int num)
{ return uiStrings::sAttribute(num); }
