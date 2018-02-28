#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "generalmod.h"
#include "factory.h"


namespace HDF5
{

    typedef od_int64		IDType;

    typedef IDType		GroupID;
    typedef BufferStringSet	GroupPath;

    inline uiString		sHDF5Err()
				{ return od_static_tr("HDF5","HDF5 Error"); }

    mExpClass(General) Reader
    {
    public:

	virtual uiRetVal	open(const char*)			= 0;
	virtual void		getGroups(const GroupPath&,
					  BufferStringSet&) const	= 0;
	virtual GroupID		groupIDFor(const GroupPath&) const	= 0;

	//etc

    };

    mExpClass(General) Writer
    {
    public:

	virtual uiRetVal	open(const char*)			= 0;

	//etc

    };

    mExpClass(General) AccessProvider
    {
    public:

	mDefineFactory0ParamInClass(AccessProvider,factory);

	virtual Reader*		getReader() const			= 0;
	virtual Writer*		getWriter() const			= 0;


    };

    inline bool isAvailable() { return !AccessProvider::factory().isEmpty(); }

} // namespace HDF5
