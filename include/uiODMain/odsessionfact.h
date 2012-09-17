#ifndef odsessionfact_h
#define odsessionfact_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id: odsessionfact.h,v 1.3 2010/08/19 07:09:04 cvsranojay Exp $
________________________________________________________________________

-*/

#include "odsession.h"

defineTranslatorGroup(ODSession,ODSessionTranslator::keyword());
defineTranslator(dgb,ODSession,mDGBKey);


#endif
