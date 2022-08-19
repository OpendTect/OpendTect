/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geeditorimpl.h"

namespace Geometry 
{

ElementEditorImpl::ElementEditorImpl( Element& elem,
	const Coord3& dir1d, const Coord3& norm2d, bool allow3d )
    : ElementEditor( elem )
    , translate1ddir( dir1d )
    , translation2dnormal( norm2d )
    , maytranslate3d( allow3d )
{
    elem.nrpositionnotifier.notify( mCB(this,ElementEditorImpl,addedKnots) );
}


ElementEditorImpl::~ElementEditorImpl()
{
    element.nrpositionnotifier.remove( mCB(this,ElementEditorImpl,addedKnots) );
}


bool ElementEditorImpl::mayTranslate1D( GeomPosID ) const
{ return translate1ddir.isDefined(); }


Coord3 ElementEditorImpl::translation1DDirection( GeomPosID ) const
{ return translate1ddir; }


bool ElementEditorImpl::mayTranslate2D( GeomPosID ) const
{ return translation2dnormal.isDefined(); }


Coord3 ElementEditorImpl::translation2DNormal( GeomPosID ) const
{ return translation2dnormal; }


bool ElementEditorImpl::mayTranslate3D( GeomPosID ) const
{ return maytranslate3d; }


void ElementEditorImpl::addedKnots(CallBacker*)
{ editpositionchange.trigger(); }

} // namespace Geometry
