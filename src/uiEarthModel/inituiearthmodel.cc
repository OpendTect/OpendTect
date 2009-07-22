/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiearthmodel.cc,v 1.3 2009-07-22 16:01:39 cvsbert Exp $";

#include "inituiearthmodel.h"
#include "uisurfaceposprov.h"

void uiEarthModel::initStdClasses()
{
    uiSurfacePosProvGroup::initClass();
}
