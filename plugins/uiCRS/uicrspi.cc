/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicrssystem.h"

#include "odplugin.h"
#include "od_ostream.h"
#include "plugins.h"

extern "C" PluginInfo* GetCRSPluginInfo(void);


namespace Coords {

using createuiCRSFromUiParentFn = uiCoordSystem*(*)(uiParent*);
mGlobal(Basic) void setGlobal_uiCRS_Fns(createuiCRSFromUiParentFn);

} // namespace Coords


mDefODPluginInfo(uiCRS)
{
    static BufferString infostr;
    infostr.set(
	"User interface for providing a library of Coordinate Reference Systems"
	" that can be set at Survey level")
	   .addNewLine(2)
	   .add("Using PROJ version: ").add( Coords::getProjVersion() );

    static PluginInfo retpi( "Coordinate Reference System (GUI)", infostr );
    return &retpi;
}


mDefODInitPlugin(uiCRS)
{
    const PluginInfo* crsinfo = GetCRSPluginInfo();
    if ( !crsinfo )
	od_cout() << "[Error] Cannot get CRS plugin information";

    const PluginManager::Data* crsdata = crsinfo ?
	    PIM().findDataWithDispName( crsinfo->dispname_ ) : nullptr;
    static BufferString msg;
    if ( !crsdata || !crsdata->isloaded_ )
    {
	msg.set( "[ERROR] The " )
	   .add( crsinfo ? crsinfo->productname_ : "CRS" )
	   .add( " plugin does not appear to be loaded\n" );
	return msg.str();
    }

#ifndef OD_NO_PROJ
    Coords::uiProjectionBasedSystem::initClass();
    setGlobal_uiCRS_Fns( Coords::uiGeodeticCoordSystem::getCRSGeodeticFld );
#endif

    return nullptr;
}
