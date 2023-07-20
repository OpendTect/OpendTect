#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "gendefs.h"

class IOObj;
class MultiID;
class VelocityDesc;

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
