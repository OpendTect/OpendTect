#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2016
________________________________________________________________________

-*/

#include "refcount.h"
#include "namedobj.h"


/*\!brief SharedObject with a name, sharable through ref counting. */

mExpClass(Basic) SharedObject : public NamedCallBacker
			      , public ReferencedObject
{
public:
				SharedObject(const char* nm=nullptr);
				SharedObject(const SharedObject&);

    static Notifier<SharedObject>&	instanceCreated();

protected:

    virtual		~SharedObject();
};
