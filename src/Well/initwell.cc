/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id: initwell.cc,v 1.2 2011-08-23 06:54:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "initwell.h"

#include "wellt2dtransform.h"

void Well::initStdClasses()
{
    mIfNotFirstTime( return );

    WellT2DTransform::initClass();
}
