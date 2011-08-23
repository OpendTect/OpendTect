/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
 ________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituimpe.cc,v 1.6 2011-08-23 06:54:12 cvsbert Exp $";


#include "inituimpe.h"
#include "uiemhorizoneditor.h"
#include "uihorizontracksetup.h"


void uiMPE::initStdClasses()
{
    mIfNotFirstTime( return );

    MPE::uiEMHorizonEditor::initClass();
    MPE::uiBaseHorizonSetupGroup::initClass();
}

