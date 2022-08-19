/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visdepthtabplanedragger.h"

#include "dragcontroller.h"
#include "vistransform.h"
#include "position.h"
#include "ranges.h"
#include "iopar.h"
#include "keyenum.h"
#include "mouseevent.h"
#include "survinfo.h"

#include <osgGeo/TabPlaneDragger>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Switch>
#include <osg/LightModel>
#include <osg/BlendFunc>
#include <osg/Version>
#include <osg/Material>

mCreateFactoryEntry( visBase::DepthTabPlaneDragger );

namespace visBase
{

class PlaneDraggerCallbackHandler : public osgManipulator::DraggerCallback
{

public:

    PlaneDraggerCallbackHandler( DepthTabPlaneDragger& dragger )
	: dragger_(dragger)
	, moved_(false)
	, dodragcontrol_(false)
    {}

    using		osgManipulator::DraggerCallback::receive;
    bool		receive(const osgManipulator::MotionCommand&) override;

protected:


    void			constrain(bool translated,bool is1d);

    void			initDragControl();
    void			applyDragControl(Coord3& newcenter);

    DepthTabPlaneDragger&	dragger_;

    osg::Matrix			initialosgmatrix_;
    Coord3			initialcenter_;
    bool			moved_;

    DragController		dragcontroller_;
    double			maxdragdist_;
    bool			dodragcontrol_;
};


bool PlaneDraggerCallbackHandler::receive(
				    const osgManipulator::MotionCommand& cmd )
{
    if ( cmd.getStage() == osgManipulator::MotionCommand::START )
    {
	initialosgmatrix_ = dragger_.osgdragger_->getMatrix();
	initialcenter_ = dragger_.center();
    }

    mDynamicCastGet( const osgManipulator::Scale1DCommand*, s1d, &cmd );
    mDynamicCastGet( const osgManipulator::Scale2DCommand*, s2d, &cmd );
    mDynamicCastGet( const osgManipulator::TranslateInLineCommand*,
		     translatedinline, &cmd );
    mDynamicCastGet( const osgManipulator::TranslateInPlaneCommand*,
		     translatedinplane, &cmd );

    bool ignore = !s1d && !s2d && !translatedinline && !translatedinplane;

    const TabletInfo* ti = TabletInfo::currentState();
    if ( ti && ti->maxPostPressDist()<5 )
	ignore = true;

    if ( ignore )
    {
	dragger_.osgdragger_->setMatrix( initialosgmatrix_ );
	return true;
    }

    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
    {
	initDragControl();
	dragger_.started.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::MOVE )
    {
	moved_ = true;
	constrain( translatedinline||translatedinplane, translatedinline||s1d );
	dragger_.motion.trigger();
    }
    else if ( moved_ && cmd.getStage()==osgManipulator::MotionCommand::FINISH )
    {
	moved_ = false;
	dragger_.finished.trigger();
	if ( initialosgmatrix_ != dragger_.osgdragger_->getMatrix() )
	    dragger_.changed.trigger();
    }

    return true;
}


void PlaneDraggerCallbackHandler::constrain( bool translated, bool is1d )
{
    Coord3 center = dragger_.center();
    Coord3 scale = dragger_.size();

    if ( translated && is1d )
	applyDragControl( center );

    for ( int dim=0; dim<3; dim++ )
    {
	if ( dragger_.spaceranges_[dim].width(false) > 0.0 )
	{
	    double diff = center[dim] - dragger_.spaceranges_[dim].start;
	    diff -= dim==dragger_.dim_ ? 0.0 : 0.5*scale[dim];
	    if ( diff < 0.0 )
	    {
		center[dim] -= translated ? diff : 0.5*diff;
		if ( !translated )
		    scale[dim] += diff;
	    }

	    diff = center[dim] - dragger_.spaceranges_[dim].stop;
	    diff += dim==dragger_.dim_ ? 0.0 : 0.5*scale[dim];
	    if ( diff > 0.0 )
	    {
		center[dim] -= translated ? diff : 0.5*diff;
		if ( !translated )
		    scale[dim] -= diff;
	    }
	}

	if ( translated || dim==dragger_.dim_ )
	    continue;

	if ( dragger_.widthranges_[dim].width(false) > 0.0 )
	{
	    double diff = scale[dim] - dragger_.widthranges_[dim].start;
	    if ( diff < 0 )
	    {
		if ( center[dim] < initialcenter_[dim] )
		    center[dim] -= 0.5*diff;
		else
		    center[dim] += 0.5*diff;

		scale[dim] -= diff;
	    }

	    diff = scale[dim] - dragger_.widthranges_[dim].stop;
	    if ( diff > 0 )
	    {
		if ( center[dim] > initialcenter_[dim] )
		    center[dim] -= 0.5*diff;
		else
		    center[dim] += 0.5*diff;

		scale[dim] -= diff;
	    }
	}
    }

    dragger_.setOsgMatrix( scale, center );
}


void PlaneDraggerCallbackHandler::initDragControl()
{
    if ( !dodragcontrol_ )
    {
	for ( int idx=0; idx<3; idx++ )
	{
	    if ( dragger_.dragctrlspacing_[idx].isUdf() )
		return;
	}

	dodragcontrol_ = true;
    }

    const Coord pos =
	    Conv::to<Coord>( dragger_.osgdragger_->getPositionOnScreen() );

    const int dim = dragger_.getDim();

    const double scalefactor = dragger_.dragctrlspacing_[dim].step;
    const Coord3 dragdir( dim==0, dim==1, dim==2 );
    dragcontroller_.init( pos, scalefactor, dragdir );
    maxdragdist_ = mUdf(double);

    const bool frontalview =
	    dragger_.osgdragger_->getPlaneNormalAngleToCamera() < M_PI/18.0;

    Coord screendragprojvec = Conv::to<Coord>( frontalview ?
		    dragger_.osgdragger_->getUpwardPlaneAxisProjOnScreen() :
		    dragger_.osgdragger_->getPlaneNormalProjOnScreen() );

    if ( screendragprojvec.sqAbs() )
    {
	screendragprojvec /= screendragprojvec.sqAbs();

	// Empirical: always move plane to camera if mouse drags downwards
	if ( dim==0 || (dim==2 && !SI().isRightHandSystem()) )
	    screendragprojvec = -screendragprojvec;

	const float dragdepth = dragger_.dragctrlspacing_[dim].width();
	screendragprojvec *= fabs(dragdepth) / scalefactor;
	dragcontroller_.dragInScreenSpace( frontalview, screendragprojvec );
    }
}


void PlaneDraggerCallbackHandler::applyDragControl( Coord3& newcenter )
{
    if ( !dodragcontrol_ )
	return;

    Coord3 dragvec = newcenter - initialcenter_;

    const Coord pos =
	    Conv::to<Coord>( dragger_.osgdragger_->getPositionOnScreen() );
    dragcontroller_.transform( dragvec, pos, maxdragdist_ );
    newcenter = initialcenter_ + dragvec;

    maxdragdist_ = -initialcenter_[dragger_.dim_];
    if ( dragvec[dragger_.dim_] < 0.0 )
	maxdragdist_ += dragger_.spaceranges_[dragger_.dim_].start;
    else
	maxdragdist_ += dragger_.spaceranges_[dragger_.dim_].stop;
}


//=============================================================================


DepthTabPlaneDragger::DepthTabPlaneDragger()
    : VisualObjectImpl( false )
    , dim_( 2 )
    , started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , osgdragger_( 0 )
    , osgdraggerplane_( 0 )
    , osgcallbackhandler_( 0 )
{
    initOsgDragger();

    centers_ += center(); centers_ += center(); centers_ += center();
    sizes_ += size(); sizes_ += size(); sizes_ += size();

    setDim(dim_);

    for ( int dim=0; dim<3; dim++ )
	dragctrlspacing_[dim].setUdf();
}


DepthTabPlaneDragger::~DepthTabPlaneDragger()
{
    osgdragger_->removeDraggerCallback( osgcallbackhandler_ );
    if ( osgcallbackhandler_ )
	osgcallbackhandler_->unref();
}


void DepthTabPlaneDragger::initOsgDragger()
{
    if ( osgdragger_ )
	return;

#if OSG_MIN_VERSION_REQUIRED(3,1,3)
    osgdragger_ = new osgGeo::TabPlaneDragger( 12.0 );
    osgdragger_->setIntersectionMask( cDraggerIntersecTraversalMask() );
    osgdragger_->setActivationMouseButtonMask(
				osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON |
				osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON );
#else
    osgdragger_ = new osgGeo::TabPlaneDragger();
#endif

    addChild( osgdragger_ );

    osgdragger_->setupDefaultGeometry();
    osgdragger_->setHandleEvents( true );

    osgcallbackhandler_ = new PlaneDraggerCallbackHandler( *this );
    osgcallbackhandler_->ref();
    osgdragger_->addDraggerCallback( osgcallbackhandler_ );

    osgdragger_->getOrCreateStateSet()->setAttributeAndModes(
		    new osg::PolygonOffset(-1.0,-1.0),
		    osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );
    osgdragger_->getOrCreateStateSet()->setRenderingHint(
					    osg::StateSet::TRANSPARENT_BIN );

    for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	osgManipulator::Dragger* dragger = osgdragger_->getDragger( idx );
	mDynamicCastGet( osgManipulator::Scale1DDragger*, s1dd, dragger );
	if ( s1dd )
	{
	    s1dd->setColor( osg::Vec4(0.0,0.7,0.0,1.0) );
	    s1dd->setPickColor( osg::Vec4(0.0,1.0,0.0,1.0) );
	}
	mDynamicCastGet( osgManipulator::Scale2DDragger*, s2dd, dragger );
	if ( s2dd )
	{
	    s2dd->setColor( osg::Vec4(0.0,0.7,0.0,1.0) );
	    s2dd->setPickColor( osg::Vec4(0.0,1.0,0.0,1.0) );
	}
    }

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back( osg::Vec3(-0.5,0.0,0.5) );
    vertices->push_back( osg::Vec3(-0.5,0.0,-0.5) );
    vertices->push_back( osg::Vec3(0.5,0.0,-0.5) );
    vertices->push_back( osg::Vec3(0.5,0.0,0.5) );

    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    normals->push_back( osg::Vec3(0.0,1.0,0.0) );

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back( osg::Vec4(0.7,0.7,0.7,0.5) );

    osg::ref_ptr<osg::Geometry> plane = new osg::Geometry;
    plane->setVertexArray( vertices.get() );
    plane->setNormalArray( normals.get() );
    plane->setNormalBinding( osg::Geometry::BIND_OVERALL );
    plane->setColorArray( colors.get() );
    plane->setColorBinding( osg::Geometry::BIND_OVERALL );

    plane->addPrimitiveSet( new osg::DrawArrays(GL_QUADS,0,4) );
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable( plane.get() );
    geode->getOrCreateStateSet()->setMode( GL_BLEND, osg::StateAttribute::ON );
    geode->getStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    geode->getStateSet()->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
    geode->getStateSet()->setAttributeAndModes(
		    new osg::PolygonOffset(1.0,1.0),
		    osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );
    osgdragger_->getStateSet()->setAttributeAndModes(
		    new osg::Material,
		    osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );
    geode->setNodeMask( Math::SetBits( geode->getNodeMask(),
			    visBase::cDraggerIntersecTraversalMask(), false) );

    osgdraggerplane_ = new osg::Switch();
    osgdraggerplane_->addChild( geode.get() );
    osgdragger_->addChild( osgdraggerplane_ );

    showPlane( false );
    showDraggerBorder( true );
}


void DepthTabPlaneDragger::setOsgMatrix( const Coord3& worldscale,
					 const Coord3& worldtrans )
{
    osg::Matrix mat;

    if ( dim_ == 0 )
	mat.makeRotate( osg::Vec3(1,0,0), osg::Vec3(0,1,0) );
    else if ( dim_ == 2 )
	mat.makeRotate( osg::Vec3(0,1,0), osg::Vec3(0,0,1) );

    osg::Vec3d scale, trans;
    mVisTrans::transformSize( transform_, worldscale, scale );
    mVisTrans::transform( transform_, worldtrans, trans );

    mat *= osg::Matrix::scale( scale );
    mat *= osg::Matrix::translate( trans );
    osgdragger_->setMatrix( mat );
}


void DepthTabPlaneDragger::setCenter( const Coord3& newcenter, bool alldims )
{
    centers_[dim_] = newcenter;

    if ( alldims )
    {
	centers_[0] = newcenter;
	centers_[1] = newcenter;
	centers_[2] = newcenter;
    }

    setOsgMatrix( size(), newcenter );
}


Coord3 DepthTabPlaneDragger::center() const
{
    Coord3 res;
    mVisTrans::transformBack( transform_,
			      osgdragger_->getMatrix().getTrans(), res );
    return res;
}


void DepthTabPlaneDragger::setSize( const Coord3& scale, bool alldims )
{
    const float abs = scale.abs();
    Coord3 newscale( scale[0] ? scale[0] : abs,
		     scale[1] ? scale[1] : abs,
		     scale[2] ? scale[2] : abs );
    sizes_[dim_] = newscale;

    if ( alldims )
    {
	sizes_[0] = newscale; sizes_[1] = newscale; sizes_[2] = newscale;
    }

    setOsgMatrix( newscale, center() );
}


Coord3 DepthTabPlaneDragger::size() const
{
    Coord3 scale;
    mVisTrans::transformBackSize( transform_,
				  osgdragger_->getMatrix().getScale(), scale );
    return scale;
}


void DepthTabPlaneDragger::removeScaleTabs()
{
    for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	osgManipulator::Dragger* dragger = osgdragger_->getDragger( idx );
	mDynamicCastGet( osgManipulator::Scale1DDragger*, s1dd, dragger );
	mDynamicCastGet( osgManipulator::Scale2DDragger*, s2dd, dragger );
	if ( s1dd || s2dd )
	{
	    osgdragger_->removeChild( dragger );
	    osgdragger_->removeDragger( dragger );
	}
    }
}


void DepthTabPlaneDragger::setDim( int newdim )
{
    centers_[dim_] = center();
    sizes_[dim_] = size();

    dim_ = newdim;

    NotifyStopper stopper( changed );
    setSize( sizes_[dim_], false );
    setCenter( centers_[dim_], false );
}


int DepthTabPlaneDragger::getDim() const
{
    return dim_;
}


void DepthTabPlaneDragger::setSpaceLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    spaceranges_[0] = x; spaceranges_[1] = y; spaceranges_[2] = z;
}


void DepthTabPlaneDragger::getSpaceLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    x = spaceranges_[0]; y = spaceranges_[1]; z = spaceranges_[2];
}


void DepthTabPlaneDragger::setWidthLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    widthranges_[0] = x; widthranges_[1] = y; widthranges_[2] = z;
}


void DepthTabPlaneDragger::getWidthLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    x = widthranges_[0]; y = widthranges_[1]; z = widthranges_[2];
}


void DepthTabPlaneDragger::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transform_ == nt )
	return;

    const Coord3 centerpos = center();
    const Coord3 savedsize = size();

    transform_ = nt;

    setSize( savedsize );
    setCenter( centerpos );
}


const mVisTrans* DepthTabPlaneDragger::getDisplayTransformation() const
{
    return transform_;
}


void DepthTabPlaneDragger::setTransDragKeys( bool depth, int ns )
{
    int mask = osgGA::GUIEventAdapter::NONE;

    if ( ns & OD::ControlButton )
	mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if ( ns & OD::ShiftButton )
	mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if ( ns & OD::AltButton )
	mask |= osgGA::GUIEventAdapter::MODKEY_ALT;

    if ( depth )
	osgdragger_->set1DTranslateModKeyMask( mask );
    else
	osgdragger_->set2DTranslateModKeyMask( mask );
}


int DepthTabPlaneDragger::getTransDragKeys( bool depth ) const
{
    const int mask = depth ? osgdragger_->get1DTranslateModKeyMask()
			   : osgdragger_->get2DTranslateModKeyMask();

    int state = OD::NoButton;

    if ( mask & osgGA::GUIEventAdapter::MODKEY_CTRL )
	state |= OD::ControlButton;

    if ( mask & osgGA::GUIEventAdapter::MODKEY_SHIFT )
	state |= OD::ShiftButton;

    if ( mask & osgGA::GUIEventAdapter::MODKEY_ALT )
	state |= OD::AltButton;

    return (OD::ButtonState) state;
}


void DepthTabPlaneDragger::showDraggerBorder( bool yn )
{
    const float borderopacity = yn ? 1.0 : 0.0;
    for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			 osgdragger_->getDragger(idx) );
	if ( tpd )
	{
	    const osg::Vec4 col( 0.5, 0.5, 0.5, borderopacity );
	    if ( col != tpd->getTranslate2DDragger()->getColor() )
		tpd->getTranslate2DDragger()->setColor( col );

	    const osg::Vec4 pickcol( 1.0, 1.0, 1.0, borderopacity );
	    if ( pickcol != tpd->getTranslate2DDragger()->getPickColor() )
		tpd->getTranslate2DDragger()->setPickColor( pickcol );
	}
    }
}


bool DepthTabPlaneDragger::isDraggerBorderShown() const
{
    for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			 osgdragger_->getDragger(idx) );
	if ( tpd )
	    return tpd->getTranslate2DDragger()->getColor()[3];
    }

    return false;
}


void DepthTabPlaneDragger::showPlane( bool yn )
{
    if ( osgdraggerplane_ )
	osgdraggerplane_->setValue( 0, yn );
}


bool DepthTabPlaneDragger::isPlaneShown() const
{ return osgdraggerplane_ && osgdraggerplane_->getValue(0); }


void DepthTabPlaneDragger::setDragCtrlSpacing( const StepInterval<float>& x,
		 const StepInterval<float>& y, const StepInterval<float>& z )
{
    dragctrlspacing_[0] = x;
    dragctrlspacing_[1] = y;
    dragctrlspacing_[2]= z;
}


} // namespace visBase
