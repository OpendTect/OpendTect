#ifndef attrfact_h
#define attrfact_h

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

uiString AttribDescSetTranslatorGroup::sTypeName()
{ return uiStrings::sAttribute(mPlural); }


#endif
