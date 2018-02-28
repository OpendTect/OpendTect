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
	( "HDF5 Support (Base)", "HDF5 Data Access",
	mODPluginCreator, mODPluginVersion,
	"Adds HDF5-based data access" ) );
    return &retpi;
}


mDefODInitPlugin(HDF5)
{
    HDF5::AccessProviderImpl::initClass();
    return 0;
}
