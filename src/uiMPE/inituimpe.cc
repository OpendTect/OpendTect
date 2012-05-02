/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
 ________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: inituimpe.cc,v 1.8 2012-05-02 11:53:47 cvskris Exp $";


#include "moddepmgr.h"
#include "uiemhorizoneditor.h"
#include "uihorizontracksetup.h"


mDefModInitFn(uiMPE)
{
    mIfNotFirstTime( return );

    MPE::uiEMHorizonEditor::initClass();
    MPE::uiBaseHorizonSetupGroup::initClass();
}

