/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "namedmonitoredobject.h"

NamedMonitoredObject::NamedMonitoredObject( const char* nm )
    : NamedObject(nm)
{
    instanceCreated().trigger();
}


NamedMonitoredObject::NamedMonitoredObject( const NamedMonitoredObject& oth )
    : NamedObject(oth)
{
    instanceCreated().trigger();
}


NamedMonitoredObject::~NamedMonitoredObject()
{
    sendDelNotif();
}


mImplMonitorableAssignment(NamedMonitoredObject,MonitoredObject)


Notifier<NamedMonitoredObject>& NamedMonitoredObject::instanceCreated()
{
    mDefineStaticLocalObject( Notifier<NamedMonitoredObject>, theNotif, (0));
    return theNotif;
}


void NamedMonitoredObject::copyClassData( const NamedMonitoredObject& oth )
{
    name_ = oth.name_;
}

MonitoredObject::ChangeType NamedMonitoredObject::compareClassData(
					const NamedMonitoredObject& oth ) const
{
    mDeliverSingCondMonitorableCompare( name_ == oth.getName(), cNameChange() );
}


void NamedMonitoredObject::setName( const char* newnm )
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
