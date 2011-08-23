/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id: initwell.cc,v 1.3 2011-08-23 14:51:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "wellt2dtransform.h"


mDefModInitFn(Well)
{
    mIfNotFirstTime( return );

    WellT2DTransform::initClass();
}
