/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initseis.cc,v 1.3 2008-11-25 15:35:22 cvsbert Exp $";

#include "initseis.h"
#include "timedepthconv.h"
#include "seisseqio.h"

void Seis::initStdClasses()
{
    Time2DepthStretcher::initClass();
    Seis::ODSeqInp::initClass();
    Seis::ODSeqOut::initClass();
}
