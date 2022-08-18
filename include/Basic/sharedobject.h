#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
