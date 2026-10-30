/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgplanedragger.h"

#include "visvsgdataman.h"
#include "visvsgevent.h"
#include "visvsgtexturechannels.h"
#include "visvsgtransform.h"
#include "vsggeolayeredtexture.h"
#include "vsggeoplanedragger.h"

#include "coltabindex.h"
#include "coltabsequence.h"
#include "color.h"
#include "keyenum.h"
#include "math2.h"

#include <vsg/app/Camera.h>
#include <vsg/maths/transform.h>

mCreateDGBVisFactoryEntry( dGBVis::DepthTabPlaneDragger );

namespace dGBVis
{

DepthTabPlaneDragger::DepthTabPlaneDragger()
    : VisualObjectImpl(true)
    , started(this)
    , motion(this)
    , changed(this)
    , finished(this)
    , center_(0.,0.,0.)
{
    plane_ = vsgGeo::TabPlaneDragger::create();
    addChild( plane_.get() );
    setPickable( true, true );
    for ( int idx=0; idx<3; idx++ )
    {
	centers_[idx] = center_;
	sizes_[idx] = size_;
	space_[idx] = Interval<float>( -1e30f, 1e30f );
	width_[idx] = Interval<float>( 0.f, 1e30f );
	spacing_[idx] = StepInterval<float>( 0.f, 0.f, 0.f );
    }

    depthkeys_ = int(OD::NoButton);
    planekeys_ = int(OD::ShiftButton);
    pushGeometry();
}


DepthTabPlaneDragger::~DepthTabPlaneDragger()
{
    detachAllNotifiers();
}


void DepthTabPlaneDragger::removeScaleTabs()
{
    if ( plane_ )
	plane_->removeScaleTabs();
}


void DepthTabPlaneDragger::setCenter( const Coord3& pos, bool alldims )
{
    center_ = pos;
    centers_[dim_] = pos;
    if ( alldims )
    {
	for ( int idx=0; idx<3; idx++ )
	    centers_[idx] = pos;
    }

    clamp();
    pushGeometry();
    changed.trigger();
}


void DepthTabPlaneDragger::setSize( const Coord3& sz, bool alldims )
{
    size_ = sz;
    sizes_[dim_] = sz;
    if ( alldims )
    {
	for ( int idx=0; idx<3; idx++ )
	    sizes_[idx] = sz;
    }

    clamp();
    pushGeometry();
    changed.trigger();
}


void DepthTabPlaneDragger::setDim( int dim )
{
    if ( dim<0 || dim>2 )
	return;

    dim_ = dim;
    center_ = centers_[dim_];
    size_ = sizes_[dim_];
    pushGeometry();
}


void DepthTabPlaneDragger::setSpaceLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    space_[0] = x;
    space_[1] = y;
    space_[2] = z;
    clamp();
    pushGeometry();
}


void DepthTabPlaneDragger::getSpaceLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    x = space_[0];
    y = space_[1];
    z = space_[2];
}


void DepthTabPlaneDragger::setColor( const OD::Color& col )
{
    if ( plane_ )
	plane_->setColor( toVec4(col,1.f) );
}


int DepthTabPlaneDragger::nrCells()
{
    return vsgGeo::TabPlaneDragger::nrCells();
}


void DepthTabPlaneDragger::setChannels( TextureChannels* channels )
{
    if ( channels_.ptr() == channels )
	return;

    if ( channels_ )
	mDetachCB( channels_->colTabChanged,
		   DepthTabPlaneDragger::channelsChangedCB );

    channels_ = channels;
    if ( channels_ )
	mAttachCB( channels_->colTabChanged,
		   DepthTabPlaneDragger::channelsChangedCB );

    updateCellColors();
}


void DepthTabPlaneDragger::channelsChangedCB( CallBacker* )
{
    updateCellColors();
}


void DepthTabPlaneDragger::updateCellColors()
{
    if ( !plane_ || !channels_ || channels_->nrTiles()!=1 )
	return;

    const vsgGeo::LayeredTexture* texture = channels_->tile(0).texture_.get();
    const int nrcells = nrCells();
    if ( texture->width()!=nrcells || texture->height()!=nrcells )
	return;

    const ColTab::Sequence& seq = channels_->getColTabSequence();
    const ColTab::IndexedLookUpTable table( seq, TextureChannels::nrSteps() );
    TypeSet<vsg::vec4> colors;
    for ( int idy=0; idy<nrcells; idy++ )
    {
	for ( int idx=0; idx<nrcells; idx++ )
	{
	    const OD::Color col = texture->isUndefAt(idx,idy)
				? seq.undefColor()
				: table.colorForIndex( texture->indexAt(idx,idy) );
	    colors += toVec4( col );
	}
    }

    plane_->setCellColors( colors.arr() );
}


void DepthTabPlaneDragger::setWidthLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    width_[0] = x;
    width_[1] = y;
    width_[2] = z;
    clamp();
    pushGeometry();
}


void DepthTabPlaneDragger::getWidthLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    x = width_[0];
    y = width_[1];
    z = width_[2];
}


void DepthTabPlaneDragger::setDisplayTransformation( const mDGBVisTrans* trans )
{
    displaytrans_ = const_cast<mDGBVisTrans*>( trans );
    pushGeometry();
}


void DepthTabPlaneDragger::showDraggerBorder( bool yn )
{
    bordershown_ = yn;
    if ( plane_ )
	plane_->setShowBorder( yn );
}


void DepthTabPlaneDragger::showPlane( bool yn )
{
    planeshown_ = yn;
    if ( plane_ )
	plane_->setShowPlane( yn );
}


void DepthTabPlaneDragger::setTransDragKeys( bool depth, int keys )
{
    if ( depth )
	depthkeys_ = keys;
    else
	planekeys_ = keys;
}


int DepthTabPlaneDragger::getTransDragKeys( bool depth ) const
{
    return depth ? depthkeys_ : planekeys_;
}


void DepthTabPlaneDragger::setDragCtrlSpacing(
			const StepInterval<float>& x,
			const StepInterval<float>& y,
			const StepInterval<float>& z )
{
    spacing_[0] = x;
    spacing_[1] = y;
    spacing_[2] = z;
}


void DepthTabPlaneDragger::setEventCatcher( EventCatcher* catcher )
{
    if ( events_ == catcher )
	return;

    if ( events_ )
	mDetachCB( events_->eventhappened, DepthTabPlaneDragger::eventCB );

    events_ = catcher;
    if ( catcher )
	mAttachCB( catcher->eventhappened, DepthTabPlaneDragger::eventCB );
}


void DepthTabPlaneDragger::setNodeTransforms( const Transformation* zstretch,
					      const Transformation* rotation )
{
    zstretch_ = const_cast<Transformation*>( zstretch );
    rotation_ = const_cast<Transformation*>( rotation );
}


void DepthTabPlaneDragger::clamp()
{
    for ( int idx=0; idx<3; idx++ )
    {
	if ( space_[idx].width(false) > 0.f )
	{
	    const double half = idx==dim_ ? 0. :
					    0.5 * Math::Abs( size_[idx] );
	    double diff = center_[idx] - space_[idx].start_ - half;
	    if ( diff < 0. )
		center_[idx] -= diff;

	    diff = center_[idx] - space_[idx].stop_ + half;
	    if ( diff > 0. )
		center_[idx] -= diff;
	}

	if ( size_[idx] < width_[idx].start_ )
	    size_[idx] = width_[idx].start_;
	if ( size_[idx] > width_[idx].stop_ )
	    size_[idx] = width_[idx].stop_;
    }

    centers_[dim_] = center_;
    sizes_[dim_] = size_;
}


void DepthTabPlaneDragger::pushGeometry()
{
    if ( !plane_ )
	return;

    ConstRefMan<mDGBVisTrans> displaytrans = displaytrans_.get();
    Coord3 dispcenter = center_;
    Coord3 dispsize = size_;
    Transformation::transform( displaytrans.ptr(), center_, dispcenter );
    Transformation::transformSize( displaytrans.ptr(), size_, dispsize );
    plane_->setNormalAxis( dim_ );
    plane_->setCenter( vsg::dvec3(dispcenter.x_,dispcenter.y_,dispcenter.z_) );
    plane_->setSize( vsg::dvec3(Math::Abs(dispsize.x_),Math::Abs(dispsize.y_),
				Math::Abs(dispsize.z_)) );
    plane_->setShowPlane( planeshown_ );
    plane_->setShowBorder( bordershown_ );
}


bool DepthTabPlaneDragger::sceneToObject( const Coord3& scenept,
					  Coord3& objectpt ) const
{
    ConstRefMan<Transformation> zstretch = zstretch_.get();
    ConstRefMan<Transformation> rotation = rotation_.get();
    ConstRefMan<mDGBVisTrans> displaytrans = displaytrans_.get();
    Coord3 localpt = scenept;
    if ( zstretch || rotation )
    {
	vsg::dmat4 world = zstretch ? zstretch->getMatrix()
				    : rotation->getMatrix();
	if ( zstretch && rotation )
	    world = zstretch->getMatrix() * rotation->getMatrix();

	const vsg::dvec4 local = vsg::inverse( world ) *
		    vsg::dvec4( scenept.x_, scenept.y_, scenept.z_, 1. );
	localpt = Coord3( local.x, local.y, local.z );
    }

    Transformation::transformBack( displaytrans.ptr(), localpt, objectpt );
    return objectpt.isDefined();
}


bool DepthTabPlaneDragger::objectToScene( const Coord3& objectpt,
					  Coord3& scenept ) const
{
    ConstRefMan<Transformation> zstretch = zstretch_.get();
    ConstRefMan<Transformation> rotation = rotation_.get();
    ConstRefMan<mDGBVisTrans> displaytrans = displaytrans_.get();
    Coord3 localpt;
    Transformation::transform( displaytrans.ptr(), objectpt, localpt );
    if ( zstretch || rotation )
    {
	vsg::dmat4 world = zstretch ? zstretch->getMatrix()
				    : rotation->getMatrix();
	if ( zstretch && rotation )
	    world = zstretch->getMatrix() * rotation->getMatrix();

	const vsg::dvec4 scene = world *
		    vsg::dvec4( localpt.x_, localpt.y_, localpt.z_, 1. );
	scenept = Coord3( scene.x, scene.y, scene.z );
    }
    else
	scenept = localpt;

    return scenept.isDefined();
}


bool DepthTabPlaneDragger::objectPoint( const EventInfo& ev,
					Coord3& objectpt ) const
{
    if ( !ev.mouseline.direction(false).isDefined() &&
	 ev.displaypickedpos.isDefined() )
	return sceneToObject( ev.displaypickedpos, objectpt );

    const Coord3 start = ev.mouseline.getPoint( 0. );
    const Coord3 end = ev.mouseline.getPoint( 1. );
    Coord3 ostart, oend;
    if ( !sceneToObject(start,ostart) || !sceneToObject(end,oend) )
	return false;

    const Coord3 dir = oend - ostart;
    const Coord3 normal( dim_==0?1.:0., dim_==1?1.:0., dim_==2?1.:0. );
    const double denom = normal.dot( dir );
    if ( Math::Abs(denom) < 1e-12 )
	return false;

    const double t = normal.dot( center_-ostart ) / denom;
    objectpt = ostart + dir * t;
    return objectpt.isDefined();
}


bool DepthTabPlaneDragger::rayInObject( const EventInfo& ev,
					Coord3& start, Coord3& end ) const
{
    if ( !ev.mouseline.direction(false).isDefined() )
	return false;

    return sceneToObject( ev.mouseline.getPoint(0.), start ) &&
	   sceneToObject( ev.mouseline.getPoint(1.), end );
}


static bool projectToWindow( const vsg::Camera& camera, const Coord3& scene,
			     Coord& pixel )
{
    if ( !camera.projectionMatrix || !camera.viewMatrix )
	return false;

    const vsg::dmat4 toclip = camera.projectionMatrix->transform() *
			      camera.viewMatrix->transform();
    const vsg::dvec4 clip = toclip *
		vsg::dvec4( scene.x_, scene.y_, scene.z_, 1. );
    if ( Math::Abs(clip.w) < 1e-12 )
	return false;

    const VkViewport& viewport = camera.getViewport();
    const double width = viewport.width>0. ? viewport.width : 1.;
    const double height = viewport.height>0. ? viewport.height : 1.;
    pixel.x_ = (clip.x/clip.w * 0.5 + 0.5) * width + viewport.x;
    pixel.y_ = (clip.y/clip.w * 0.5 + 0.5) * height + viewport.y;
    return pixel.isDefined();
}


void DepthTabPlaneDragger::initNormalProjector( const EventInfo& ev )
{
    dragplaneok_ = false;
    dragmouse_ = ev.mousepos;
    dragpixelsperunit_ = 1.;
    dragfrontsign_ = 1.;

    Coord3 start, end;
    if ( !rayInObject(ev,start,end) )
	return;

    const Coord3 dir = end - start;
    const Coord3 axis( dim_==0 ? 1. : 0.,
		       dim_==1 ? 1. : 0.,
		       dim_==2 ? 1. : 0. );
    const Coord3 lateral = dir - axis * axis.dot( dir );
    if ( lateral.sqAbs() > 0.03*dir.sqAbs() )
    {
	const double denom = lateral.dot( dir );
	if ( Math::Abs(denom) > 1e-12 )
	{
	    const double s = lateral.dot( dragcenter_-start ) / denom;
	    const Coord3 hit = start + dir * s;
	    dragplanenml_ = lateral;
	    dragaxisref_ = hit[dim_];
	    dragplaneok_ = true;
	    return;
	}
    }

    const vsg::Camera* camera = events_ ? events_->camera() : nullptr;
    if ( !camera )
	return;

    Coord3 sceneside, scenecenter;
    Coord3 side = dragcenter_;
    side[(dim_+1)%3] += 1.;
    Coord pixcenter, pixside;
    if ( !objectToScene(dragcenter_,scenecenter) ||
	 !objectToScene(side,sceneside) ||
	 !projectToWindow(*camera,scenecenter,pixcenter) ||
	 !projectToWindow(*camera,sceneside,pixside) )
	return;

    dragpixelsperunit_ = pixcenter.distTo( pixside );
    if ( Math::Abs(dragpixelsperunit_) < 1e-3 )
	dragpixelsperunit_ = 1.;

    const vsg::dmat4 invview = vsg::inverse( camera->viewMatrix->transform() );
    const vsg::dvec4 forward = invview * vsg::dvec4( 0., 0., -1., 0. );
    Coord3 forwardobj;
    Coord3 localfwd( forward.x, forward.y, forward.z );
    ConstRefMan<Transformation> zstretch = zstretch_.get();
    ConstRefMan<Transformation> rotation = rotation_.get();
    ConstRefMan<mDGBVisTrans> displaytrans = displaytrans_.get();
    if ( zstretch || rotation )
    {
	vsg::dmat4 world = zstretch ? zstretch->getMatrix()
				    : rotation->getMatrix();
	if ( zstretch && rotation )
	    world = zstretch->getMatrix() * rotation->getMatrix();
	const vsg::dvec4 d = vsg::inverse( world ) *
		    vsg::dvec4( forward.x, forward.y, forward.z, 0. );
	localfwd = Coord3( d.x, d.y, d.z );
    }

    Transformation::transformBackDir( displaytrans.ptr(),
				      localfwd, forwardobj );
    if ( forwardobj[dim_] < 0. )
	dragfrontsign_ = -1.;
}


bool DepthTabPlaneDragger::normalDragPoint( const EventInfo& ev,
					    Coord3& objectpt ) const
{
    objectpt = dragstart_;
    if ( dragplaneok_ )
    {
	Coord3 start, end;
	if ( !rayInObject(ev,start,end) )
	    return false;

	const Coord3 dir = end - start;
	const double denom = dragplanenml_.dot( dir );
	if ( Math::Abs(denom) < 1e-12 )
	    return false;

	const double s = dragplanenml_.dot( dragcenter_-start ) / denom;
	const Coord3 hit = start + dir * s;
	objectpt[dim_] = dragstart_[dim_] + (hit[dim_] - dragaxisref_);
	return objectpt.isDefined();
    }

    if ( !ev.mousepos.isDefined() || !dragmouse_.isDefined() )
	return false;

    const double dy = ev.mousepos.y_ - dragmouse_.y_;
    objectpt[dim_] = dragstart_[dim_] +
		     dy / dragpixelsperunit_ * dragfrontsign_;
    return true;
}


void DepthTabPlaneDragger::beginDrag( const Coord3& point, bool alongnormal )
{
    dragging_ = true;
    dragmoved_ = false;
    drag1d_ = alongnormal;
    dragplaneok_ = false;
    dragstart_ = point;
    dragcenter_ = center_;
    dragsize_ = size_;
    started.trigger();
}


void DepthTabPlaneDragger::dragTo( const Coord3& point )
{
    if ( !dragging_ )
	return;

    Coord3 delta = point - dragstart_;
    if ( drag1d_ )
    {
	Coord3 next = dragcenter_;
	next[dim_] = dragcenter_[dim_] + delta[dim_];
	center_ = next;
    }
    else
    {
	delta[dim_] = 0.;
	center_ = dragcenter_ + delta;
    }

    clamp();
    pushGeometry();
    motion.trigger();
    changed.trigger();
}


void DepthTabPlaneDragger::endDrag()
{
    if ( !dragging_ )
	return;

    dragging_ = false;
    finished.trigger();
}


void DepthTabPlaneDragger::eventCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const EventInfo&,ev,cb);
    const bool hitself = !ev.pickedobjids.isEmpty() &&
			 ev.pickedobjids[0]==id();
    const int buttons = int(ev.buttonstate_);
    if ( (buttons & int(OD::LeftButton)) == 0 )
    {
	if ( dragging_ && ev.type==MouseClick && !ev.pressed )
	    endDrag();
	return;
    }

    if ( !dragging_ && !hitself )
	return;

    const bool normalmove = dragging_ && drag1d_;
    Coord3 point;
    if ( !normalmove && !objectPoint(ev,point) )
	return;

    if ( ev.type==MouseClick && ev.pressed && !dragging_ )
    {
	const int modmask = int(OD::ShiftButton) | int(OD::ControlButton)
			  | int(OD::AltButton);
	const int held = buttons & modmask;
	const bool alongnormal = held==(depthkeys_ & modmask);
	if ( !alongnormal && held!=(planekeys_ & modmask) )
	    return;

	beginDrag( point, alongnormal );
	if ( alongnormal )
	    initNormalProjector( ev );
	if ( events_ )
	    events_->setHandled();
    }
    else if ( ev.type==MouseMovement && dragging_ )
    {
	if ( drag1d_ && !normalDragPoint(ev,point) )
	    return;

	if ( (buttons & int(OD::ControlButton)) && spacing_[dim_].step_!=0.f )
	{
	    const float step = spacing_[dim_].step_;
	    const float snapped = spacing_[dim_].start_ +
		step * Math::Floor( float(point[dim_]-spacing_[dim_].start_) /
				    step + 0.5f );
	    point[dim_] = snapped;
	}

	dragmoved_ = true;
	dragTo( point );
	if ( events_ )
	    events_->setHandled();
    }
    else if ( ev.type==MouseClick && !ev.pressed && dragging_ )
    {
	const bool moved = dragmoved_;
	endDrag();
	if ( moved && events_ )
	    events_->setHandled();
    }
}

} // namespace dGBVis
