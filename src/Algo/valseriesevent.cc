/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : May 2005
-*/


#include "valseriesevent.h"

mDefineEnumUtils(VSEvent,Type,"Event type")
{
	"None",
	"Peak or trough",
	"Peak (Max)",
	"Trough (Min)",
	"Zero crossing",
	"Zero crossing - to +",
	"Zero crossing + to -",
	"Largest peak",
	"Largest trough",
	0
};
