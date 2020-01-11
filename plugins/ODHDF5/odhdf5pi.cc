/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2008
________________________________________________________________________

-*/

#include "odplugin.h"
#include "hdf5accessimpl.h"


mDefODPluginEarlyLoad(ODHDF5)
mDefODPluginInfo(ODHDF5)
{
    mDefineStaticLocalObject( PluginInfo, retpi,
	( "HDF5 Support (Base)", "OpendTect",
	"dGB", "=od",
	"HDF5 plugin" ) );
    return &retpi;
}


mDefODInitPlugin(ODHDF5)
{
    HDF5::AccessProviderImpl::initHDF5();
    return 0;
}
