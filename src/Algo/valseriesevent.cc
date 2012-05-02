/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : May 2005
-*/

static const char* mUnusedVar rcsID = "$Id: valseriesevent.cc,v 1.3 2012-05-02 11:52:56 cvskris Exp $";

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
