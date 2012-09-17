/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiwell.cc,v 1.4 2012/02/24 23:17:35 cvsnanne Exp $";

#include "moddepmgr.h"
#include "uiwellposprov.h"
#include "uiwellt2dconv.h"

mDefModInitFn(uiWell)
{
    mIfNotFirstTime( return );

    uiWellPosProvGroup::initClass();
    uiT2DWellConvSelGroup::initClass();
}
