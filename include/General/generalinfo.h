#ifndef generalinfo_h
#define generalinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		06-03-2020
 RCS:		$Id$
________________________________________________________________________

-*/


#include "generalmod.h"

#include "commondefs.h"


class BufferString;
class BufferStringSet;

namespace OD
{

mGlobal(General) bool	getHostIDs( BufferStringSet& hostids,
				    BufferString& errmsg );
} //namespace OD

#endif
