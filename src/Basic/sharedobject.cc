/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sharedobject.h"

SharedObject::SharedObject( const char* nm )
    : NamedCallBacker(nm)
{
    instanceCreated().trigger( this );
}


SharedObject::~SharedObject()
{
    sendDelNotif();
}


Notifier<SharedObject>& SharedObject::instanceCreated()
{
    mDefineStaticLocalObject( Notifier<SharedObject>, theNotif, (0) );
    return theNotif;
}
