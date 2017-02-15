/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-9-1995
-*/


#include "namedobj.h"
#include "iopar.h"
#include "keystrs.h"
#include <ctype.h>

mDefineInstanceCreatedNotifierAccess(NamedMonitorable);


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


void NamedObject::putNameInPar( IOPar& iop ) const
{
    iop.set( sKey::Name(), getName() );
}


NamedMonitorable::NamedMonitorable( const char* nm )
    : NamedObject(nm)
{
    mTriggerInstanceCreatedNotifier();
}


NamedMonitorable::NamedMonitorable( const NamedMonitorable& oth )
    : NamedObject(oth)
{
    mTriggerInstanceCreatedNotifier();
}


NamedMonitorable::~NamedMonitorable()
{
    sendDelNotif();
}


mImplMonitorableAssignment(NamedMonitorable,Monitorable)

void NamedMonitorable::copyClassData( const NamedMonitorable& oth )
{
    name_ = oth.name_;
}

Monitorable::ChangeType NamedMonitorable::compareClassData(
					const NamedMonitorable& oth ) const
{
    mDeliverSingCondMonitorableCompare( name_ == oth.getName(), cNameChange() );
}


void NamedMonitorable::setName( const char* newnm )
{
    mLock4Read();
    if ( name_ == newnm )
	return;
    if ( !mLock2Write() && name_ == newnm )
	return;

    ChangeData cd( cNameChange(), 0 );
    cd.auxdata_ = new NameChgData( name_, newnm );
    name_ = newnm;
    sendChgNotif( mAccessLocker(), cd );
}
