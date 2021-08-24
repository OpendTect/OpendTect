/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-9-1995
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


NamedCallBacker::NamedCallBacker( const NamedCallBacker& oth )
    : NamedObject(oth)
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
