/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          December 2007
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituimpe.cc,v 1.3 2008-11-25 15:35:25 cvsbert Exp $";


#include "inituimpe.h"
#include "uiemhorizoneditor.h"
#include "uihorizontracksetup.h"


void uiMPE::initStdClasses()
{
    MPE::uiEMHorizonEditor::initClass();
    MPE::uiHorizonSetupGroup::initClass();
}

