/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id$
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
    dgbWellTranslator::initClass();
    
    WellT2DTransform::initClass();
    Pos::WellProvider3D::initClass();
}
