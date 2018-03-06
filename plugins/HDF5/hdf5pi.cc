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
	( "HDF5 Support", "HDF5 File Access",
	mODPluginCreator, mODPluginVersion,
	"Adds HDF5-based data access" ) );
    retpi.useronoffselectable_ = true;
    retpi.url_ = "hdfgroup.org";
    mSetPackageDisplayName( retpi, HDF5::Access::sHDF5PackageDispName() );
    retpi.uidispname_ = retpi.uipackagename_;
    return &retpi;
}


mDefODInitPlugin(HDF5)
{
    HDF5::AccessProviderImpl::initHDF5();
    return 0;
}
