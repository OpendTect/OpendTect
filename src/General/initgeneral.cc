/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: initgeneral.cc,v 1.2 2008-02-05 14:25:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "initgeneral.h"
#include "rectposprovider.h"
#include "tableposprovider.h"

void General::initStdClasses()
{
    Pos::RectProvider3D::initClass();
    Pos::RectProvider2D::initClass();
    Pos::TableProvider3D::initClass();
}
