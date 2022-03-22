/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uihorizonattrib.h"

mDefModInitFn(uiEMAttrib)
{
    mIfNotFirstTime( return );

    uiHorizonAttrib::initClass();
}
