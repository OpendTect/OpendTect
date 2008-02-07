/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: initgeneral.cc,v 1.4 2008-02-07 16:10:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "initgeneral.h"
#include "rangeposprovider.h"

void General::initStdClasses()
{
    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
}
