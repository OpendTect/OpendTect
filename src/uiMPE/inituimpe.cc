/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          December 2007
 RCS:           $Id: inituimpe.cc,v 1.2 2008-02-20 20:19:33 cvskris Exp $
 ________________________________________________________________________

-*/


#include "inituimpe.h"
#include "uiemhorizoneditor.h"
#include "uihorizontracksetup.h"


void uiMPE::initStdClasses()
{
    MPE::uiEMHorizonEditor::initClass();
    MPE::uiHorizonSetupGroup::initClass();
}

