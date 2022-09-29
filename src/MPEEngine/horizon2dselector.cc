/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizon2dselector.h"

#include "ptrman.h"
#include "survinfo.h"
#include "emhorizon2d.h"

namespace MPE
{

Horizon2DSelector::Horizon2DSelector( const EM::Horizon2D& hor )
    : SectionSourceSelector()
    , horizon_( hor )
{}


Horizon2DSelector::~Horizon2DSelector()
{}


int Horizon2DSelector::nextStep()
{
    pErrMsg( "Implement" );
    return 0;
}

} // namespace MPE
