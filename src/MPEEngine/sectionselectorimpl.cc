/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/


#include "sectionselectorimpl.h"

#include "emhorizon3d.h"


namespace MPE
{

BinIDSurfaceSourceSelector::BinIDSurfaceSourceSelector(
	const EM::Horizon3D& hor, const EM::SectionID& sid )
    : SectionSourceSelector( sid )
    , surface_( hor )
{}


SurfaceSourceSelector::SurfaceSourceSelector(
	const EM::EMObject& obj, const EM::SectionID& sid )
    : SectionSourceSelector( sid )
    , emobject_( obj )
{}

} // namespace MPE
