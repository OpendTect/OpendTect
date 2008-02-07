/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: initgeneral.cc,v 1.3 2008-02-07 14:53:54 cvsbert Exp $
________________________________________________________________________

-*/

#include "initgeneral.h"
#include "rectposprovider.h"

void General::initStdClasses()
{
    Pos::RectProvider3D::initClass();
    Pos::RectProvider2D::initClass();
}
