/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiwell.cc,v 1.2 2011-08-23 06:54:12 cvsbert Exp $";

#include "inituiwell.h"
#include "uiwellt2dconv.h"

void uiWell::initStdClasses()
{
    mIfNotFirstTime( return );

    uiT2DWellConvSelGroup::initClass();
}
