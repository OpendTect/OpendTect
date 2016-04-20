/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-9-1995
-*/


#include "namedobj.h"
#include "iopar.h"
#include "keystrs.h"
#include <ctype.h>

mDefineInstanceCreatedNotifierAccess(NamedObject);


NamedObject::NamedObject( const char* nm )
    : name_(nm)
{
    mTriggerInstanceCreatedNotifier();
}


NamedObject::NamedObject( const NamedObject& oth )
    : name_(oth.name())
{
    mTriggerInstanceCreatedNotifier();
}


NamedObject::~NamedObject()
{
    sendDelNotif();
}


bool NamedObject::operator ==( const NamedObject& oth ) const
{
    mLock4Access();
    return name_ == oth.getName();
}


BufferString NamedObject::getName() const
{
    mLock4Access();
    return name_;
}


void NamedObject::setName( const char* nm )
{
    mLock4Access();
    if ( name_ != nm )
    {
	name_ = nm;
	mSendChgNotif();
    }
}


bool NamedObject::getNameFromPar( const IOPar& iop )
{
    BufferString myname( name() );
    if ( !iop.get(sKey::Name(),myname) )
	return false;
    setName( myname );
    return true;
}


void NamedObject::putNameInPar( IOPar& iop ) const
{
    iop.set( sKey::Name(), getName() );
}
