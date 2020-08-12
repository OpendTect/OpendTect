#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id$
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


