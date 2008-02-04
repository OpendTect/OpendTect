/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: initgeneral.cc,v 1.1 2008-02-04 16:21:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "initgeneral.h"
#include "tableposprovider.h"
// #include "polyposprovider.h"

void General::initStdClasses()
{
    Pos::TableProvider3D::initClass();
    // Pos::PolyProvider3D::initClass();
}
