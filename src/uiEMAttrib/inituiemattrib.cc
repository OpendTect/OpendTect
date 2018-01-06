/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2018
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "uihorizonattrib.h"

mDefModInitFn(uiEMAttrib)
{
    mIfNotFirstTime( return );

    uiHorizonAttrib::initClass();
}
