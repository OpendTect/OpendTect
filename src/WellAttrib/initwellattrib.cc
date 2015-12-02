/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "welllogattrib.h"

mDefModInitFn(WellAttrib)
{
    mIfNotFirstTime( return );

    Attrib::WellLog::initClass();
}
