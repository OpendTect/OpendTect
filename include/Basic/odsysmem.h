#ifndef odsysmem_h
#define odsysmem_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2012
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"
#include "gendefs.h"
class IOPar;

namespace OD
{
    mGlobal(Basic) void	getSystemMemory(od_int64& total,od_int64& free);
    mGlobal(Basic) void	dumpMemInfo(IOPar&);
}


#endif

