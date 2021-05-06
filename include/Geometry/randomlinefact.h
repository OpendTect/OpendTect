#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		November 2006
________________________________________________________________________

-*/
#include "uistrings.h"

defineTranslatorGroup(RandomLineSet,"RandomLine Geometry");
defineTranslator(dgb,RandomLineSet,mDGBKey);

uiString RandomLineSetTranslatorGroup::sTypeName( int num )
{ return uiStrings::sRandomLine( num ); }


