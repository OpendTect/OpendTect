/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "namedobj.h"
#include "iopar.h"
#include "keystrs.h"
#include <ctype.h>



void ObjectWithName::putNameInPar( IOPar& iop ) const
{
    iop.set( sKey::Name(), name() );
}


NamedObject& NamedObject::operator =( const NamedObject& oth )
{
    if ( this != &oth )
	name_ = oth.getName();
    return *this;
}


bool NamedObject::getNameFromPar( const IOPar& iop )
{
    BufferString myname( getName() );
    if ( !iop.get(sKey::Name(),myname) )
	return false;
    setName( myname );
    return true;
}


NamedCallBacker::NamedCallBacker( const char* nm )
    : NamedObject(nm)
    , delnotif_(this)
    , delalreadytriggered_(false)
{
}


void NamedCallBacker::sendDelNotif() const
{
    if ( !delalreadytriggered_ )
    {
	delalreadytriggered_ = true;
	objectToBeDeleted().trigger();
    }
}
