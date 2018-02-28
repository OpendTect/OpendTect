#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2018
________________________________________________________________________

-*/

#include "hdf5common.h"


namespace HDF5
{

mExpClass(HDF5) AccessProviderImpl : public AccessProvider
{
public:

    mDefaultFactoryInstantiation0Param( AccessProvider, AccessProviderImpl,
			"OD", toUiString("OD") );

    virtual Reader*	getReader() const;
    virtual Writer*	getWriter() const;

};

} // namespace HDF5
