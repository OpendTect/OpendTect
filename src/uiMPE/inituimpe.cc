/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituimpe.cc,v 1.7 2011/08/23 14:51:33 cvsbert Exp $";


#include "moddepmgr.h"
#include "uiemhorizoneditor.h"
#include "uihorizontracksetup.h"


mDefModInitFn(uiMPE)
{
    mIfNotFirstTime( return );

    MPE::uiEMHorizonEditor::initClass();
    MPE::uiBaseHorizonSetupGroup::initClass();
}

