/*+
________________________________________________________________________

 Copyright:	dGB Beheer B.V.
 License:	https://dgbes.com/index.php/licensing
 Author:	Nanne Hemstra
 Date:		September 2021
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "horizonattrib.h"

mDefModInitFn(EMAttrib)
{
    mIfNotFirstTime( return );

    Attrib::Horizon::initClass();
}
