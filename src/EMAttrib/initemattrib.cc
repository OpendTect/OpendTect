/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:          December 2017
________________________________________________________________________

-*/


#include "moddepmgr.h"
#include "horizonattrib.h"

mDefModInitFn(EMAttrib)
{
    mIfNotFirstTime( return );

    Attrib::Horizon::initClass();
}
