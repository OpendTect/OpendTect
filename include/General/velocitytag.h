#ifndef velocitytag_h
#define velocitytag_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2013
 RCS:		$Id: ibmformat.h 28906 2013-03-20 08:26:48Z mahant.mothey@dgbes.com $
________________________________________________________________________

-*/

#include "generalmod.h"
#include "commondefs.h"

class IOObj;
class VelocityDesc;

namespace Vel
{
    
/*! Writes velocity information to an IOObj. */
mGlobal(General) bool SetStorageTag(IOObj&,const VelocityDesc&);


/*! Removes velocity information from an IOObj. */
mGlobal(General) bool RemoveStorageTag(IOObj&);
    

/*! Reads velocity information from an IOObj. */
mGlobal(General) bool GetStorageTag(const IOObj&,VelocityDesc&);

}; // Namespace

#endif

