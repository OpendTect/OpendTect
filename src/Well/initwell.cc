/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "wellt2dtransform.h"
#include "wellposprovider.h"
#include "welltransl.h"


mDefModInitFn(Well)
{
    mIfNotFirstTime( return );

    WellTranslatorGroup::initClass();
    odWellTranslator::initClass();
    
    WellT2DTransform::initClass();
    Pos::WellProvider3D::initClass();
}
