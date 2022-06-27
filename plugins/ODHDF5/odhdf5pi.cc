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
    BufferString version;
    version.add( H5_VERS_MAJOR ).add(".")
	   .add( H5_VERS_MINOR ).add(".")
	   .add( H5_VERS_RELEASE );
    mDeclStaticString( infostr );
    infostr.set( "HDF5 plugin" ).addNewLine(2)
	   .add( "Using HDF5 version: " ).add( version );

    mDefineStaticLocalObject( PluginInfo, retpi, (
	"HDF5 Support (Base)",
	"OpendTect",
	"dGB Earth Sciences",
	"=od",
	infostr.buf() ))
    return &retpi;
}


mDefODInitPlugin(ODHDF5)
{
    HDF5::AccessProviderImpl::initHDF5();
    return nullptr;
}
