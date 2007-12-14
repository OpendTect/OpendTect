/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          December 2007
 RCS:           $Id: inituimpe.cc,v 1.1 2007-12-14 05:10:16 cvssatyaki Exp $
 ________________________________________________________________________

-*/


#include "inituimpe.h"
#include "uiemhorizoneditor.h"
#include "uifaulttracksetup.h"
#include "uihorizontracksetup.h"


void uiMPE::initStdClasses()
{
    MPE::uiEMHorizonEditor::initClass();
    MPE::uiHorizonSetupGroup::initClass();
    MPE::uiFaultSetupGroup::initClass();
}

