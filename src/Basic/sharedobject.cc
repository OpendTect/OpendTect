/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		January 2022
________________________________________________________________________

-*/


#include "sharedobject.h"

SharedObject::SharedObject( const char* nm )
    : NamedCallBacker(nm)
{
    instanceCreated().trigger( this );
}


SharedObject::SharedObject( const SharedObject& oth )
    : NamedCallBacker(oth)
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
