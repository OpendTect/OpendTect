/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituimpe.cc,v 1.4 2009-07-22 16:01:40 cvsbert Exp $";


#include "inituimpe.h"
#include "uiemhorizoneditor.h"
#include "uihorizontracksetup.h"


void uiMPE::initStdClasses()
{
    MPE::uiEMHorizonEditor::initClass();
    MPE::uiHorizonSetupGroup::initClass();
}

