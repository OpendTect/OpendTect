#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2016
________________________________________________________________________

-*/

#include "namedobj.h"


/*\!brief Monitorable object with a name, sharable through ref counting. */

mExpClass(Basic) SharedObject :	public RefCount::Referenced
			      , public NamedMonitorable
{
public:

			SharedObject(const char* nm=0);
			mDeclMonitorableAssignment(SharedObject);
    bool		operator ==( const SharedObject& oth ) const
			{ return NamedMonitorable::operator ==( oth ); }

    mDeclInstanceCreatedNotifierAccess(SharedObject);

protected:

    virtual		~SharedObject();

};
