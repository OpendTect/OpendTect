/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initseis.cc,v 1.4 2008-12-09 21:38:44 cvskris Exp $";

#include "initseis.h"
#include "timedepthconv.h"
#include "seisseqio.h"

void Seis::initStdClasses()
{
    Time2DepthStretcher::initClass();
    Depth2TimeStretcher::initClass();
    Seis::ODSeqInp::initClass();
    Seis::ODSeqOut::initClass();
}
