/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sectionselectorimpl.h"

#include "emhorizon3d.h"


namespace MPE
{

// BinIDSurfaceSourceSelector
BinIDSurfaceSourceSelector::BinIDSurfaceSourceSelector(
						const EM::Horizon3D& hor )
    : SectionSourceSelector()
    , surface_( hor )
{}


BinIDSurfaceSourceSelector::~BinIDSurfaceSourceSelector()
{}


int BinIDSurfaceSourceSelector::nextStep()
{
    return 0;
}


// SurfaceSourceSelector
SurfaceSourceSelector::SurfaceSourceSelector( const EM::EMObject& obj )
    : SectionSourceSelector()
    , emobject_( obj )
{}


SurfaceSourceSelector::~SurfaceSourceSelector()
{}


int SurfaceSourceSelector::nextStep()
{
    return 0;
}

} // namespace MPE
