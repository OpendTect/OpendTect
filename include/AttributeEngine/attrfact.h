#ifndef attrfact_h
#define attrfact_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: attrfact.h,v 1.2 2006-08-30 16:03:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "attribdescsettr.h"

#define sKeyAttribDescSetTranslatorGroup "Attribute definitions"

defineTranslatorGroup(AttribDescSet,sKeyAttribDescSetTranslatorGroup);
defineTranslator(dgb,AttribDescSet,mDGBKey);


#endif
