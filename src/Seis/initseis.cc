/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initseis.cc,v 1.5 2009-07-22 16:01:34 cvsbert Exp $";

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
