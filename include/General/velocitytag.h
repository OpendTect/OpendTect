#ifndef velocitytag_h
#define velocitytag_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2013
 RCS:		$Id$
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

