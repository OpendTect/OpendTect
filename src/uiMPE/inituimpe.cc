/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
 ________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "moddepmgr.h"
#include "uiemhorizoneditor.h"
#include "uihorizontracksetup.h"


mDefModInitFn(uiMPE)
{
    mIfNotFirstTime( return );

    MPE::uiEMHorizonEditor::initClass();
    MPE::uiBaseHorizonSetupGroup::initClass();
}

