/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiwell.cc,v 1.3 2011-08-23 14:51:33 cvsbert Exp $";

#include "moddepmgr.h"
#include "uiwellt2dconv.h"

mDefModInitFn(uiWell)
{
    mIfNotFirstTime( return );

    uiT2DWellConvSelGroup::initClass();
}
