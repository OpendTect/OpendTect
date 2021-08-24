/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2006
___________________________________________________________________

-*/


#include "horizon2dselector.h"

#include "ptrman.h"
#include "survinfo.h"
#include "emhorizon2d.h"

namespace MPE
{

Horizon2DSelector::Horizon2DSelector( const EM::Horizon2D& hor,
				      const EM::SectionID& sid )
    : SectionSourceSelector( sid )
    , horizon_( hor )
{}


int Horizon2DSelector::nextStep()
{
    pErrMsg( "Implement" );
    return 0;
}

} // namespace MPE
