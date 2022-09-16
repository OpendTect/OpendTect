#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"

class StringPairSet;
namespace OD
{
    mGlobal(Basic) void	getSystemMemory(od_int64& total,od_int64& free);
    mGlobal(Basic) void	dumpMemInfo(StringPairSet&);

    mDeprecated ("Use the StringPairSet version instead")
    mGlobal(Basic) void	dumpMemInfo(IOPar&);
}
