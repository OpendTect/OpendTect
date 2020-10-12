#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2016
________________________________________________________________________

-*/

#include "namedmonitoredobject.h"


/*\!brief MonitoredObject with a name, sharable through ref counting. */

mExpClass(Basic) SharedObject :	public RefCount::Referenced
			      , public NamedMonitoredObject
{
public:

			SharedObject(const char* nm=0);
			mDeclAbstractMonitorableAssignment(SharedObject);

    mDeclInstanceCreatedNotifierAccess(SharedObject);

protected:

    virtual		~SharedObject();

};
