/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: initseis.cc,v 1.2 2008-02-02 14:05:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "initseis.h"
#include "timedepthconv.h"
#include "seisseqio.h"

void Seis::initStdClasses()
{
    Time2DepthStretcher::initClass();
    Seis::ODSeqInp::initClass();
    Seis::ODSeqOut::initClass();
}
