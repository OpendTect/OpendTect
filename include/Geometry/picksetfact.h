#ifndef picksetfact_h
#define picksetfact_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		May 2001
 RCS:		$Id$
________________________________________________________________________

-*/

#include "picksettr.h"
#include "uistrings.h"

defineTranslatorGroup(PickSet,"PickSet Group");
defineTranslator(dgb,PickSet,mDGBKey);

uiString PickSetTranslatorGroup::sTypeName( int num)
{ return uiStrings::sPickSet( num ); }

#endif

