/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeotrackball.h"

#include <vsg/app/ViewMatrix.h>
#include <vsg/maths/transform.h>
#include <vsg/ui/PointerEvent.h>
#include <vsg/ui/ScrollWheelEvent.h>

#include <algorithm>
#include <cmath>

namespace vsgGeo
{

static vsg::ButtonMask buttonBit( uint32_t button )
{
    if ( button==1 )
	return vsg::BUTTON_MASK_1;
    if ( button==2 )
	return vsg::BUTTON_MASK_2;
    if ( button==3 )
	return vsg::BUTTON_MASK_3;
    return vsg::BUTTON_MASK_OFF;
}


Trackball::Trackball( vsg::ref_ptr<vsg::Camera> camera )
    : Inherit(camera)
{}


void Trackball::enableDragging( vsg::ButtonMask mask )
{
    if ( mask==dragmask_ )
	return;

    dragmask_ = mask;
    if ( !modeAllowed() )
	endDrag();
}


void Trackball::setLeftButtonPans( bool yn )
{
    if ( leftpans_==yn )
	return;

    leftpans_ = yn;
    if ( yn )
    {
	rotateButtonMask = vsg::BUTTON_MASK_OFF;
	panButtonMask = vsg::ButtonMask( int(vsg::BUTTON_MASK_1) |
					  int(vsg::BUTTON_MASK_2) );
    }
    else
    {
	rotateButtonMask = vsg::BUTTON_MASK_1;
	panButtonMask = vsg::BUTTON_MASK_2;
    }
}


void Trackball::setMovementCallback( const MovementCB& cb )
{
    movementcb_ = cb;
}


Trackball::Pose Trackball::pose() const
{
    Pose ret;
    if ( _lookAt )
    {
	ret.eye_ = _lookAt->eye;
	ret.center_ = _lookAt->center;
	ret.up_ = _lookAt->up;
    }

    return ret;
}


void Trackball::turn( double angle, bool horizontal )
{
    if ( !_lookAt )
	return;

    const vsg::dvec3 dir = _lookAt->eye - _lookAt->center;
    const vsg::dvec3 axis = horizontal ? _lookAt->up
				       : vsg::cross( dir, _lookAt->up );
    const double axislen = vsg::length( axis );
    if ( axislen < 1e-12 )
	return;

    const vsg::dmat4 rotation = vsg::rotate( angle, axis/axislen );
    const vsg::dvec4 newdir = rotation * vsg::dvec4( dir.x, dir.y, dir.z, 0. );
    _lookAt->eye = _lookAt->center + vsg::dvec3( newdir.x, newdir.y, newdir.z );
    if ( horizontal )
	return;

    const vsg::dvec3 up = _lookAt->up;
    const vsg::dvec4 newup = rotation * vsg::dvec4( up.x, up.y, up.z, 0. );
    _lookAt->up = vsg::dvec3( newup.x, newup.y, newup.z );
}


void Trackball::changeDistance( double factor )
{
    if ( !_lookAt )
	return;

    const double scale = std::max( 1e-3, 1. + factor );
    _lookAt->eye = _lookAt->center + (_lookAt->eye-_lookAt->center) * scale;
}


void Trackball::rotate( double angle, const vsg::dvec3& axis )
{
    const Pose before = pose();
    Inherit::rotate( angle, axis );
    notifyMovement( before );
}


void Trackball::zoom( double ratio )
{
    const Pose before = pose();
    Inherit::zoom( ratio );
    notifyMovement( before );
}


void Trackball::notifyMovement( const Pose& before )
{
    if ( !movementcb_ || !_lookAt )
	return;

    const vsg::dvec3 olddir = before.eye_ - before.center_;
    const vsg::dvec3 newdir = _lookAt->eye - _lookAt->center;
    const double oldlen = vsg::length( olddir );
    const double newlen = vsg::length( newdir );
    if ( oldlen<1e-12 || newlen<1e-12 )
	return;

    float horangle = 0.f;
    float vertangle = 0.f;
    const vsg::dvec3 oldunit = olddir / oldlen;
    const vsg::dvec3 newunit = newdir / newlen;
    const vsg::dvec3 turnvec = vsg::cross( oldunit, newunit );
    const double sinangle = vsg::length( turnvec );
    if ( sinangle > 1e-12 )
    {
	const double angle = std::atan2( sinangle, vsg::dot(oldunit,newunit) );
	const vsg::dvec3 axis = turnvec / sinangle;
	const double uplen = vsg::length( before.up_ );
	if ( uplen > 1e-12 )
	    horangle = float( -angle * vsg::dot(before.up_/uplen,axis) );

	const vsg::dvec3 vercross = vsg::cross( oldunit, before.up_ );
	const double verlen = vsg::length( vercross );
	if ( verlen > 1e-12 )
	    vertangle = float( -angle * vsg::dot(vercross/verlen,axis) );
    }

    const float distfactor = float( (newlen-oldlen)/oldlen );
    if ( horangle!=0.f || vertangle!=0.f || distfactor!=0.f )
	movementcb_( horangle, vertangle, distfactor, wheelstep_ );
}


void Trackball::apply( vsg::ScrollWheelEvent& ev )
{
    wheelstep_ = true;
    Inherit::apply( ev );
    wheelstep_ = false;
}


void Trackball::apply( vsg::ButtonPressEvent& ev )
{
    if ( ev.handled )
	return;

    const vsg::ButtonMask bit = buttonBit( ev.button );
    if ( (dragmask_ & bit)==0 )
	return;

    Inherit::apply( ev );
}


void Trackball::apply( vsg::ButtonReleaseEvent& ev )
{
    if ( ev.handled )
    {
	endDrag();
	return;
    }

    Inherit::apply( ev );
}


void Trackball::apply( vsg::MoveEvent& ev )
{
    if ( ev.handled || !modeAllowed() )
    {
	endDrag();
	return;
    }

    Inherit::apply( ev );
}


bool Trackball::modeAllowed() const
{
    if ( _updateMode==ROTATE )
	return (dragmask_ & vsg::BUTTON_MASK_1)!=0;
    if ( _updateMode==PAN )
	return (dragmask_ & vsg::BUTTON_MASK_2)!=0;
    if ( _updateMode==ZOOM )
	return (dragmask_ & vsg::BUTTON_MASK_3)!=0;
    return true;
}


void Trackball::endDrag()
{
    _updateMode = INACTIVE;
    _thrown = false;
    _previousPointerEvent = {};
}

} // namespace vsgGeo
