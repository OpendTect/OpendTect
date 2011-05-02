/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituimpe.cc,v 1.5 2011-05-02 06:10:06 cvsumesh Exp $";


#include "inituimpe.h"
#include "uiemhorizoneditor.h"
#include "uihorizontracksetup.h"


void uiMPE::initStdClasses()
{
    MPE::uiEMHorizonEditor::initClass();
    MPE::uiBaseHorizonSetupGroup::initClass();
}

