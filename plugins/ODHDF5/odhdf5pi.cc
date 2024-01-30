/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odplugin.h"
#include "hdf5accessimpl.h"
#include "hdf5writerimpl.h"
#include "legal.h"


namespace HDF5
{

using voidFromH5StringPairFn = void(*)(const H5::H5Object&,const char* from,
				       const char* to,uiRetVal&);
mGlobal(General) void setGlobal_General_Fns(voidFromH5StringPairFn);

} // namespace HDF5



static uiString* legalText()
{
    return legalText( "hdf5" );
}


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

    static PluginInfo retpi(
	"HDF5 Support (Base)",
	infostr.buf() );
    return &retpi;
}


mDefODInitPlugin(ODHDF5)
{
    HDF5::AccessProviderImpl::initHDF5();
    legalInformation().addCreator( legalText, "HDF5" );
    HDF5::setGlobal_General_Fns( HDF5::WriterImpl::renObjImpl );
    return nullptr;
}
