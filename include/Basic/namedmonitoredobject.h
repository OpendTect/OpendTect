#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "namedobj.h"
#include "monitor.h"


/*!\brief MonitoredObject with a name. All but name() are MT-safe. */

mExpClass(Basic) NamedMonitoredObject : public MonitoredObject
				      , public NamedObject
{
public:

			NamedMonitoredObject(const char* nm=0);
			NamedMonitoredObject(const NamedObject&);
    virtual		~NamedMonitoredObject();
    mDeclMonitorableAssignment(NamedMonitoredObject);

    inline		mImplSimpleMonitoredGetOverride(getName,
							BufferString,name_)
    void		setName(const char*) override;

    static Notifier<NamedMonitoredObject>&	instanceCreated();

    static ChangeType	cNameChange()			{ return 1; }

    mExpClass(Basic) NameChgData : public ChangeData::AuxData
    {
    public:
			NameChgData( const char* from, const char* to )
			    : oldnm_(from), newnm_(to)	{}

	BufferString	oldnm_;
	BufferString	newnm_;

    };
};
