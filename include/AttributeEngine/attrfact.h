#ifndef attrfact_h
#define attrfact_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: attrfact.h,v 1.3 2009/07/22 16:01:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "attribdescsettr.h"

#define sKeyAttribDescSetTranslatorGroup "Attribute definitions"

defineTranslatorGroup(AttribDescSet,sKeyAttribDescSetTranslatorGroup);
defineTranslator(dgb,AttribDescSet,mDGBKey);


#endif
