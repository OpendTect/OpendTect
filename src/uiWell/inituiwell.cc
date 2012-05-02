/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: inituiwell.cc,v 1.6 2012-05-02 15:12:27 cvskris Exp $";

#include "moddepmgr.h"
#include "uiwellposprov.h"
#include "uiwellt2dconv.h"

mDefModInitFn(uiWell)
{
    mIfNotFirstTime( return );

    uiWellPosProvGroup::initClass();
    uiT2DWellConvSelGroup::initClass();
}
