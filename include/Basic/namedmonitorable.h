#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2017
________________________________________________________________________

-*/

#include "basicmod.h"
#include "namedobj.h"
#include "monitor.h"


/*!\brief Monitorable object with a name. All but name() are MT-safe. */

mExpClass(Basic) NamedMonitorable : public Monitorable
				  , public NamedObject
{
public:

			NamedMonitorable(const char* nm=0);
			NamedMonitorable(const NamedObject&);
    virtual		~NamedMonitorable();
    mDeclMonitorableAssignment(NamedMonitorable);

    inline		mImplSimpleMonitoredGetOverride(getName,
							BufferString,name_)
    void		setName(const char*) override;

    mDeclInstanceCreatedNotifierAccess(NamedMonitorable);

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
