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
class MultiID;
class BufferString;

/*! Writes velocity information to an IOObj. */
mGlobal(General) bool SetVelocityTag(IOObj&,const VelocityDesc&);


/*! Removes velocity information from an IOObj. */
mGlobal(General) bool RemoveVelocityTag(IOObj&);
    

/*! Reads velocity information from an IOObj. */
mGlobal(General) bool GetVelocityTag(const IOObj&,VelocityDesc&);


/*! Writes velocity volume information to an IOObj. */
mGlobal(General) bool SetVelocityVolumeTag(IOObj&,const MultiID&);


/*! Removes velocity volume information from an IOObj. */
mGlobal(General) bool RemoveVelocityVolumeTag(IOObj&);
    

/*! Reads velocity volume information from an IOObj. */
mGlobal(General) bool GetVelocityVolumeTag(const IOObj&,MultiID&);


/*! Writes zdomain information to an IOObj. */
mGlobal(General) bool SetZDomainTag(IOObj&,const char* zdomain);


/*! Writes depth information to an IOObj. */
mGlobal(General) bool SetDepthTag(IOObj&,const MultiID* velocity=0);


/*! Writes time information to an IOObj. */
mGlobal(General) bool SetTimeTag(IOObj&,const MultiID* velocity=0);


/*! Reads zdomain information from an IOObj. */
mGlobal(General) bool GetZDomainTag(const IOObj&,BufferString&);


/*! Removes zdomain information from an IOObj. */
mGlobal(General) bool RemoveZDomainTag(IOObj&);

#endif

