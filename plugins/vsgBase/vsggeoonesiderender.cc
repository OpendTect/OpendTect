/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoonesiderender.h"

#include <vsg/app/RecordTraversal.h>
#include <vsg/maths/transform.h>
#include <vsg/vk/State.h>

namespace vsgGeo
{

OneSideRender::OneSideRender()
{
}


void OneSideRender::setLine( size_t idx, const vsg::dvec3& pos,
			     const vsg::dvec3& dir )
{
    if ( idx >= lines_.size() )
	lines_.resize( idx+1, Line{ vsg::dvec3(), vsg::dvec3(0.,0.,1.) } );

    lines_[idx] = Line{ pos, dir };
}


bool OneSideRender::isDrawn( size_t idx, const vsg::dvec3& eye ) const
{
    if ( idx >= lines_.size() )
	return true;

    const Line& line = lines_[idx];
    return vsg::dot( eye-line.pos_, line.dir_ ) >= 0.;
}


void OneSideRender::accept( vsg::RecordTraversal& rt ) const
{
    const vsg::State* state = rt.getState();
    const vsg::dmat4 inv =
		vsg::inverse( state->modelviewMatrixStack.top() );
    const vsg::dvec3 eye = inv * vsg::dvec3( 0., 0., 0. );

    for ( size_t idx=0; idx<children.size(); idx++ )
    {
	if ( children[idx] && isDrawn(idx,eye) )
	    children[idx]->accept( rt );
    }
}

} // namespace vsgGeo
