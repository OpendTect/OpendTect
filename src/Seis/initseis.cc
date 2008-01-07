/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: initseis.cc,v 1.1 2008-01-07 22:36:35 cvskris Exp $
________________________________________________________________________

-*/

#include "initseis.h"
#include "timedepthconv.h"

void Seis::initStdClasses()
{
    Time2DepthStretcher::initClass();
}
