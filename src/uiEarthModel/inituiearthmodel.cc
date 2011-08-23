/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiearthmodel.cc,v 1.5 2011-08-23 14:51:33 cvsbert Exp $";

#include "moddepmgr.h"
#include "uisurfaceposprov.h"

mDefModInitFn(uiEarthModel)
{
    mIfNotFirstTime( return );

    uiSurfacePosProvGroup::initClass();
}
