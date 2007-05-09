
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : Mar 2007
-*/

static const char* rcsID = "$Id: tutseistools.cc,v 1.1 2007-05-09 15:58:49 cvsbert Exp $";

#include "tutseistools.h"
#include "seistrcprop.h"


Tut::SeisTools::SeisTools( float fact, float shft )
    : factor_(fact), shift_(shft)
{
}


void Tut::SeisTools::apply( SeisTrc& trc ) const
{
    SeisTrcPropChg pc( trc );
    pc.scale( factor_, shift_ );
}
