/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/

#include "odplugin.h"
#include "hdf5accessimpl.h"


mDefODPluginEarlyLoad(HDF5)
mDefODPluginInfo(HDF5)
{
    mDefineStaticLocalObject( PluginInfo, retpi,
	( "HDF5 Support (Base)", "HDF5 File Access",
	mODPluginCreator, mODPluginVersion,
	mODPluginSeeMainModDesc ) );
    return &retpi;
}


mDefODInitPlugin(HDF5)
{
    HDF5::AccessProviderImpl::initHDF5();
    return 0;
}
