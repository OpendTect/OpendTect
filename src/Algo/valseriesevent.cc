/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : May 2005
-*/

static const char* rcsID mUnusedVar = "$Id: valseriesevent.cc,v 1.4 2012-05-02 15:11:20 cvskris Exp $";

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
