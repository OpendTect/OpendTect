/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "uicrssystem.h"

#include "odplugin.h"

namespace Coords {

using createuiCRSFromUiParentFn = uiCoordSystem*(*)(uiParent*);
mGlobal(Basic) void setGlobal_uiCRS_Fns(createuiCRSFromUiParentFn);

} // namespace Coords


mDefODPluginInfo(uiCRS)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Coordinate Reference System (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Raman)",
	"=od",
	"User interface for providing a library of Coordinate Reference Systems"
	    " that can be set at Survey level" ));
    return &retpi;
}


mDefODInitPlugin(uiCRS)
{
#ifndef OD_NO_PROJ
    Coords::uiProjectionBasedSystem::initClass();
    setGlobal_uiCRS_Fns( Coords::uiGeodeticCoordSystem::getCRSGeodeticFld );
#endif

    return nullptr;
}
