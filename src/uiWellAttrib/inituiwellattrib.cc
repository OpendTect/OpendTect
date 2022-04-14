/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "uiodstratlayermodelmgr.h"
#include "uiwelllogattrib.h"

mDefModInitFn(uiWellAttrib)
{
    mIfNotFirstTime( return );

    uiStratLayerModelManager::initClass();
    uiWellLogAttrib::initClass();
}
