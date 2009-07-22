/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgeneral.cc,v 1.6 2009-07-22 16:01:32 cvsbert Exp $";

#include "initgeneral.h"
#include "rangeposprovider.h"

void General::initStdClasses()
{
    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
}
