#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		06-03-2020
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

