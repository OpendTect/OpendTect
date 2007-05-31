/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert Bril
 * DATE     : May 2005
-*/

static const char* rcsID = "$Id: valseriesevent.cc,v 1.1 2007-05-31 22:27:11 cvskris Exp $";

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
