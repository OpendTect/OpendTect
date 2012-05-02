/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: inituiwell.cc,v 1.5 2012-05-02 11:54:03 cvskris Exp $";

#include "moddepmgr.h"
#include "uiwellposprov.h"
#include "uiwellt2dconv.h"

mDefModInitFn(uiWell)
{
    mIfNotFirstTime( return );

    uiWellPosProvGroup::initClass();
    uiT2DWellConvSelGroup::initClass();
}
