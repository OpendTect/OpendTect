#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "commondefs.h"


class BufferString;
class BufferStringSet;
class uiString;

namespace OD
{

mGlobal(General) const char*	getLmUtilFilePath(uiString* errmsg);

mGlobal(General) bool	getHostIDs( BufferStringSet& hostids,
				    BufferString& errmsg );
} // namespace OD
