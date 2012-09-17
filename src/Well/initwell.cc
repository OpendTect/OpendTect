/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id: initwell.cc,v 1.4 2012/02/24 23:15:31 cvsnanne Exp $
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "wellt2dtransform.h"
#include "wellposprovider.h"


mDefModInitFn(Well)
{
    mIfNotFirstTime( return );

    WellT2DTransform::initClass();
    Pos::WellProvider3D::initClass();
}
