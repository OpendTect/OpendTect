/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : May 2005
-*/

static const char* rcsID = "$Id: valseriesevent.cc,v 1.2 2009/07/22 16:01:29 cvsbert Exp $";

#include "valseriesevent.h"

DefineEnumNames(VSEvent,Type,0,"Event type")
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
