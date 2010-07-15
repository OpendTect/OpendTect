/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id: initwell.cc,v 1.1 2010-07-15 10:08:01 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "initwell.h"

#include "wellt2dtransform.h"

void Well::initStdClasses()
{
    WellT2DTransform::initClass();
}
