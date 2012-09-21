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

#define sKeyAttribDescSetTranslatorGroup "Attribute definitions"

defineTranslatorGroup(AttribDescSet,sKeyAttribDescSetTranslatorGroup);
defineTranslator(dgb,AttribDescSet,mDGBKey);


#endif
