/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiearthmodel.cc,v 1.4 2011-08-23 06:54:12 cvsbert Exp $";

#include "inituiearthmodel.h"
#include "uisurfaceposprov.h"

void uiEarthModel::initStdClasses()
{
    mIfNotFirstTime( return );

    uiSurfacePosProvGroup::initClass();
}
