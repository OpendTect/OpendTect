#ifndef wellfact_h
#define wellfact_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		24-11-1995
 RCS:		$Id: wellfact.h,v 1.2 2003-09-11 15:56:00 bert Exp $
________________________________________________________________________

-*/

#include "welltransl.h"

defineTranslatorGroup(Well,"Well");
defineTranslator(dgb,Well,mDGBKey);

//- Module dependencies
# include "picksetfact.h"

#endif
