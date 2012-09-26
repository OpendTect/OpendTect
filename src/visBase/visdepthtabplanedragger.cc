/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jul 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visdepthtabplanedragger.h"

#include "SoDepthTabPlaneDragger.h"
#include "vistransform.h"
#include "position.h"
#include "ranges.h"
#include "iopar.h"
#include "keyenum.h"
#include "mouseevent.h"

#include <Inventor/nodes/SoSeparator.h>

#include <osgManipulator/TabPlaneDragger>
#include <osg/Geometry>
#include <osg/Switch>
#include <osg/LightModel>
#include <osg/BlendFunc>
#include <osg/Version>

mCreateFactoryEntry( visBase::DepthTabPlaneDragger );

namespace visBase
{


static void setOsgMatrix( osgManipulator::TabPlaneDragger& osgdragger, int dim,
			  const osg::Vec3& scale, const osg::Vec3& trans )
{ 
    osg::Matrix mat;
    if ( dim == 0 )
	mat.makeRotate( osg::Vec3(1,0,0), osg::Vec3(0,1,0) );
    else if ( dim == 2 )
	mat.makeRotate( osg::Vec3(0,1,0), osg::Vec3(0,0,1) );

    mat *= osg::Matrix::scale( scale );
    mat *= osg::Matrix::translate( trans );
    osgdragger.setMatrix( mat );
}


class PlaneDraggerCallbackHandler: public osgManipulator::DraggerCallback
{

public:

    PlaneDraggerCallbackHandler( DepthTabPlaneDragger& dragger )  
	: dragger_( dragger )						{}

    using			osgManipulator::DraggerCallback::receive;
    virtual bool		receive(const osgManipulator::MotionCommand&);

protected:

    void			constrain(bool translatedinline);

    DepthTabPlaneDragger&	dragger_;
    osg::Matrix			startmatrix_;
};


bool PlaneDraggerCallbackHandler::receive(
				    const osgManipulator::MotionCommand& cmd )
{
    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
	startmatrix_ = dragger_.osgdragger_->getMatrix();

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
	dragger_.osgdragger_->setMatrix( startmatrix_ );
	return true;
    }

    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
    {
	dragger_.started.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::MOVE )
    {
	constrain( translatedinline );
	dragger_.motion.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::FINISH )
    {
	dragger_.finished.trigger();
	if ( startmatrix_ != dragger_.osgdragger_->getMatrix() )
	    dragger_.changed.trigger();
    }

    return true;
}


void PlaneDraggerCallbackHandler::constrain( bool translatedinline )
{
    osg::Vec3 scale = dragger_.osgdragger_->getMatrix().getScale();
    osg::Vec3 center = dragger_.osgdragger_->getMatrix().getTrans();

    for ( int dim=0; dim<3; dim++ )
    {
	if ( translatedinline && dim==dragger_.dim_ )
	{
	    if ( dragger_.spaceranges_[dim].width(false) > 0.0 )
	    {
		if ( center[dim] < dragger_.spaceranges_[dim].start )
		    center[dim] = dragger_.spaceranges_[dim].start;
		if ( center[dim] > dragger_.spaceranges_[dim].stop )
		    center[dim] = dragger_.spaceranges_[dim].stop;
	    }
	}
		
	if ( !translatedinline && dim!=dragger_.dim_ )
	{
	    if ( dragger_.spaceranges_[dim].width(false) > 0.0 )
	    {
		double diff = center[dim] - 0.5*scale[dim] -
			      dragger_.spaceranges_[dim].start;
		if ( diff < 0.0 )
		{
		    center[dim] -= 0.5*diff;
		    scale[dim] += diff;
		}

		diff = center[dim] + 0.5*scale[dim] -
		       dragger_.spaceranges_[dim].stop;
		if ( diff > 0.0 )
		{
		    center[dim] -= 0.5*diff;
		    scale[dim] -= diff;
		}
	    }

	    if ( dragger_.widthranges_[dim].width(false) > 0.0 )
	    {
		double diff = scale[dim] - dragger_.widthranges_[dim].start;
		if ( diff < 0 )
		{
		    if ( center[dim] < startmatrix_.getTrans()[dim] )
			center[dim] -= 0.5*diff;
		    else
			center[dim] += 0.5*diff;

		    scale[dim] -= diff;
		}

		diff = scale[dim] - dragger_.widthranges_[dim].stop;
		if ( diff > 0 )
		{
		    if ( center[dim] > startmatrix_.getTrans()[dim] )
			center[dim] -= 0.5*diff;
		    else
			center[dim] += 0.5*diff;

		    scale[dim] -= diff;
		}
	    }
	}
    }

    setOsgMatrix( *dragger_.osgdragger_, dragger_.dim_, scale, center );
}


//=============================================================================


const char* DepthTabPlaneDragger::dimstr()	{ return "Dimension"; }
const char* DepthTabPlaneDragger::sizestr()	{ return "Size."; }
const char* DepthTabPlaneDragger::centerstr()	{ return "Center."; }

DepthTabPlaneDragger::DepthTabPlaneDragger()
    : VisualObjectImpl( false )
    , dragger_( new SoDepthTabPlaneDragger )
    , rotation_( 0 )
    , transform_( 0 )
    , dim_( 2 )
    , started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , osgdragger_( 0 )
    , osgdraggerplane_( 0 )
    , osgcallbackhandler_( 0 )
{
    if ( doOsg() )
	initOsgDragger();

    centers_ += center(); centers_ += center(); centers_ += center();
    sizes_ += size(); sizes_ += size(); sizes_ += size();

    addChild( dragger_ );

    setDim(dim_);

    dragger_->addStartCallback( DepthTabPlaneDragger::startCB, this );
    dragger_->addMotionCallback( DepthTabPlaneDragger::motionCB, this );
    dragger_->addFinishCallback( DepthTabPlaneDragger::finishCB, this );
    dragger_->addValueChangedCallback(
	    		DepthTabPlaneDragger::valueChangedCB, this );

}


DepthTabPlaneDragger::~DepthTabPlaneDragger()
{
    if ( osgdragger_ )
	osgdragger_->removeDraggerCallback( osgcallbackhandler_ );

    if ( rotation_ ) rotation_->unRef();
    if ( transform_ ) transform_->unRef();

    dragger_->removeStartCallback( DepthTabPlaneDragger::startCB, this );
    dragger_->removeMotionCallback( DepthTabPlaneDragger::motionCB, this );
    dragger_->removeFinishCallback( DepthTabPlaneDragger::finishCB, this );
    dragger_->removeValueChangedCallback(
	    		DepthTabPlaneDragger::valueChangedCB, this );
}


void DepthTabPlaneDragger::initOsgDragger()
{
    if ( !doOsg() || osgdragger_ )
	return;

#if OSG_MIN_VERSION_REQUIRED(3,1,3)
    osgdragger_ = new osgManipulator::TabPlaneDragger( 12.0 );
    osgdragger_->setIntersectionMask( IntersectionTraversal );
    osgdragger_->setActivationMouseButtonMask(
	    			osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON );

#else
    osgdragger_ = new osgManipulator::TabPlaneDragger();
#endif

    addChild( osgdragger_ );

    osgdragger_->setupDefaultGeometry();
    osgdragger_->setHandleEvents( true );

    osgcallbackhandler_ = new PlaneDraggerCallbackHandler( *this );
    osgdragger_->addDraggerCallback( osgcallbackhandler_ );

    osgdragger_->getOrCreateStateSet()->setAttributeAndModes(
	    	new osg::PolygonOffset(-1.0,1.0), osg::StateAttribute::ON );
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
    colors->push_back( osg::Vec4(0.7,0.7,0.7,0.3) );

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
    geode->getOrCreateStateSet()->setAttributeAndModes(
	    	new osg::PolygonOffset(1.0,1.0), osg::StateAttribute::ON );

    osgdraggerplane_ = new osg::Switch();
    osgdraggerplane_->addChild( geode.get() );
    osgdragger_->addChild( osgdraggerplane_ );

    showPlane( false );
    showDraggerBorder( true );
}


void DepthTabPlaneDragger::setCenter( const Coord3& newcenter, bool alldims )
{
    const Coord3 dcenter = world2Dragger( newcenter, true );

    dragger_->translation.setValue( SbVec3f(dcenter.x,dcenter.y,dcenter.z) );

    centers_[dim_] = newcenter;

    if ( alldims )
    {
	centers_[0] = newcenter;
	centers_[1] = newcenter;
	centers_[2] = newcenter;
    }

    if ( osgdragger_ )
    {
	setOsgMatrix( *osgdragger_, dim_,
		      osgdragger_->getMatrix().getScale(),
		      osg::Vec3(newcenter.x,newcenter.y,newcenter.z) );
    }
}


Coord3 DepthTabPlaneDragger::center() const
{
    if ( osgdragger_ )
    {
	osg::Vec3 dragcenter = osgdragger_->getMatrix().getTrans();
	return Coord3( dragcenter[0], dragcenter[1], dragcenter[2] );
    }

    const SbVec3f res = dragger_->translation.getValue();
    return dragger2World( Coord3(res[0],res[1],res[2]), true );
}


void DepthTabPlaneDragger::setSize( const Coord3& scale, bool alldims )
{
    const float abs = scale.abs();
    Coord3 newscale( scale[0] ? scale[0] : abs,
    		     scale[1] ? scale[1] : abs,
    		     scale[2] ? scale[2] : abs );
    const Coord3 dscale = world2Dragger( newscale, false);
    dragger_->scaleFactor.setValue(SbVec3f( dscale.x/2, dscale.y/2,dscale.z/2));

    sizes_[dim_] = newscale;

    if ( alldims )
    {
	sizes_[0] = newscale; sizes_[1] = newscale; sizes_[2] = newscale;
    }

    if ( osgdragger_ )
    {
	setOsgMatrix( *osgdragger_, dim_,
		      osg::Vec3f(newscale.x,newscale.y,newscale.z),
		      osgdragger_->getMatrix().getTrans() );
    }
}


Coord3 DepthTabPlaneDragger::size() const
{
    if ( osgdragger_ )
    {
	osg::Vec3 scale = osgdragger_->getMatrix().getScale();
	return Coord3( scale[0], scale[1], scale[2] );
    }

    const SbVec3f res = dragger_->scaleFactor.getValue();
    return dragger2World( Coord3(res[0]*2,res[1]*2,res[2]*2), false );
}


void DepthTabPlaneDragger::removeScaleTabs()
{
    dragger_->setPart("greenTabsSep", 0 );

    if ( osgdragger_ )
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
}


void DepthTabPlaneDragger::setDim( int newdim )
{
    centers_[dim_] = center();
    sizes_[dim_] = size();

    Interval<float> xlim, ylim, zlim;
    getSpaceLimits( xlim, ylim, zlim );
    Interval<float> xsizelim, ysizelim, zsizelim;
    getWidthLimits( xsizelim, ysizelim, zsizelim );

    if ( !newdim )
    {
	if ( !rotation_ )
	{
	    rotation_ = Transformation::create();
	    rotation_->ref();

	    dragger_->ref();
	    removeChild( dragger_ );
	    addChild( rotation_->getInventorNode() );
	    addChild( dragger_ );
	    dragger_->unref();
	}

	rotation_->setRotation( Coord3(0,1,0), -M_PI/2 );
	rotation_->setScale( Coord3( 1, 1, -1 ) );
    }
    else if ( newdim==1 )
    {
	if ( !rotation_ )
	{
	    rotation_ = Transformation::create();
	    rotation_->ref();

	    dragger_->ref();
	    removeChild( dragger_ );
	    addChild( rotation_->getInventorNode() );
	    addChild( dragger_ );
	    dragger_->unref();
	}

	rotation_->setRotation( Coord3(1,0,0), M_PI/2 );
	rotation_->setScale( Coord3( 1, 1, -1 ) );
    }
    else
    {
        if ( rotation_ ) rotation_->reset();
    }

    dim_ = newdim;

    setSpaceLimits( xlim, ylim, zlim );
    setWidthLimits( xsizelim, ysizelim, zsizelim );
    NotifyStopper stopper( changed );
    setSize( sizes_[dim_], false );
    setCenter( centers_[dim_], false );
    stopper.restore();
}


int DepthTabPlaneDragger::getDim() const
{
    return dim_;
}


void DepthTabPlaneDragger::setSpaceLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    const Coord3 start = world2Dragger( Coord3(x.start,y.start,z.start),true );
    const Coord3 stop = world2Dragger( Coord3(x.stop,y.stop,z.stop),true );
    dragger_->minPos.setValue( start.x, start.y, start.z);
    dragger_->maxPos.setValue( stop.x, stop.y, stop.z);

    if ( osgdragger_ )
    {
	spaceranges_[0] = x; spaceranges_[1] = y; spaceranges_[2] = z;
    }
}


void DepthTabPlaneDragger::getSpaceLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    const SbVec3f dstart = dragger_->minPos.getValue();
    const SbVec3f dstop = dragger_->maxPos.getValue();
    const Coord3 start = dragger2World( Coord3(dstart[0],dstart[1],dstart[2]),
	    				true );
    const Coord3 stop = dragger2World( Coord3(dstop[0],dstop[1],dstop[2]),
	    				true );
    x.start = start.x; x.stop = stop.x;
    y.start = start.y; y.stop = stop.y;
    z.start = start.z; z.stop = stop.z;

    if ( osgdragger_ )
    {
	x = spaceranges_[0]; y = spaceranges_[1]; z = spaceranges_[2];
    }
}


void DepthTabPlaneDragger::setWidthLimits( const Interval<float>& x,
					   const Interval<float>& y,
					   const Interval<float>& z )
{
    const Coord3 start = world2Dragger( Coord3(x.start,y.start,z.start),true );
    const Coord3 stop = world2Dragger( Coord3(x.stop,y.stop,z.stop),true );
    dragger_->minSize.setValue( start.x, start.y, start.z);
    dragger_->maxSize.setValue( stop.x, stop.y, stop.z);

    if ( osgdragger_ )
    {
	widthranges_[0] = x; widthranges_[1] = y; widthranges_[2] = z;
    }
}


void DepthTabPlaneDragger::getWidthLimits( Interval<float>& x,
					   Interval<float>& y,
					   Interval<float>& z ) const
{
    const SbVec3f dstart = dragger_->minSize.getValue();
    const SbVec3f dstop = dragger_->maxSize.getValue();
    const Coord3 start = dragger2World( Coord3(dstart[0],dstart[1],dstart[2]),
	    				false );
    const Coord3 stop = dragger2World( Coord3(dstop[0],dstop[1],dstop[2]),
	    				false );
    x.start = start.x; x.stop = stop.x;
    y.start = start.y; y.stop = stop.y;
    z.start = start.z; z.stop = stop.z;

    if ( osgdragger_ )
    {
	x = widthranges_[0]; y = widthranges_[1]; z = widthranges_[2];
    }
}


void DepthTabPlaneDragger::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transform_==nt ) return;

    const Coord3 centerpos = center();
    const Coord3 savedsize = size();

    Interval<float> xlim, ylim, zlim;
    getSpaceLimits( xlim, ylim, zlim );
    Interval<float> xsizelim, ysizelim, zsizelim;
    getWidthLimits( xsizelim, ysizelim, zsizelim );


    if ( transform_ )
    {
	removeChild( const_cast<mVisTrans*>(transform_)->getInventorNode() );
	transform_->unRef();
    }

    transform_ = nt;

    if ( transform_ )
    {
	insertChild(0, const_cast<mVisTrans*>(transform_)->getInventorNode() );
	transform_->ref();
    }

    setSpaceLimits( xlim, ylim, zlim );
    setWidthLimits( xsizelim, ysizelim, zsizelim );
    setSize( savedsize );
    setCenter( centerpos );
}


const mVisTrans* DepthTabPlaneDragger::getDisplayTransformation() const
{
    return transform_;
}


void DepthTabPlaneDragger::setOwnShape( SoNode* newnode )
{
    SoSeparator* newsep = dynamic_cast<SoSeparator*>(newnode);
    if ( !newsep )
    {
	newsep = new SoSeparator;
	newsep->addChild( newnode );
    }

    dragger_->setPart("translator", newsep );
}


void DepthTabPlaneDragger::setTransDragKeys( bool depth, int ns )
{
    const bool ctrl = ns & OD::ControlButton;
    const bool shift = ns & OD::ShiftButton;
    const bool alt = ns & OD::AltButton;

    SoDepthTabPlaneDragger::Key key;

    if ( shift )
    {
	if ( ctrl )
	    key = alt ? SoDepthTabPlaneDragger::SHIFTCONTROLALT
		    : SoDepthTabPlaneDragger::SHIFTCONTROL;
	else
	    key = alt ? SoDepthTabPlaneDragger::SHIFTALT
		    : SoDepthTabPlaneDragger::SHIFT;
    }
    else
    {
	if ( ctrl )
	    key = alt ? SoDepthTabPlaneDragger::CONTROLALT
		    : SoDepthTabPlaneDragger::CONTROL;
	else
	    key = alt ? SoDepthTabPlaneDragger::ALT
		    : SoDepthTabPlaneDragger::NONE;
    }

    if ( depth ) dragger_->depthKey.setValue( key );
    else dragger_->translateKey.setValue( key );

    if ( osgdragger_ )
    {
	unsigned int mask  = osgGA::GUIEventAdapter::NONE;
	if ( ctrl )  mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
	if ( shift ) mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
	if ( alt )   mask |= osgGA::GUIEventAdapter::MODKEY_ALT;

	for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
	{
	    mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			     osgdragger_->getDragger(idx) );

	    if ( tpd && depth )
	    {
		//tpd->getTranslate1DDragger()->setActivationMouseButtonMask( osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON );
		tpd->getTranslate1DDragger()->setActivationModKeyMask( mask );
	    }
	    if ( tpd && !depth )
	    {
		//tpd->getTranslate1DDragger()->setActivationMouseButtonMask( osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON );
		tpd->getTranslate2DDragger()->setActivationModKeyMask( mask );
	    }
	}
    }
}


int DepthTabPlaneDragger::getTransDragKeys( bool depth ) const
{
    if ( osgdragger_ )
    {
	int state = OD::NoButton;
	for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
	{
	    mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			     osgdragger_->getDragger(idx) );
	    if ( !tpd )
		continue;

	    const osgManipulator::Dragger* dragger;
	    if ( depth )
		dragger = tpd->getTranslate1DDragger();
	    else 
		dragger = tpd->getTranslate2DDragger();

	    const unsigned int ctrl  = osgGA::GUIEventAdapter::MODKEY_CTRL;
	    if ( (dragger->getActivationModKeyMask() & ctrl) == ctrl )
		state |= OD::ControlButton;

	    const unsigned int shift = osgGA::GUIEventAdapter::MODKEY_SHIFT;
	    if ( (dragger->getActivationModKeyMask() & shift) == shift )
		state |= OD::ShiftButton;

	    const unsigned int alt   = osgGA::GUIEventAdapter::MODKEY_ALT;
	    if ( (dragger->getActivationModKeyMask() & alt) == alt )
		state |= OD::AltButton;
	}
	return (OD::ButtonState) state;
    }

    SoDepthTabPlaneDragger::Key key = depth
	? (SoDepthTabPlaneDragger::Key) dragger_->depthKey.getValue()
	: (SoDepthTabPlaneDragger::Key) dragger_->translateKey.getValue();

    int state = 0;

    if ( key==SoDepthTabPlaneDragger::SHIFTCONTROLALT ||
	key==SoDepthTabPlaneDragger::SHIFTCONTROL ||
	key==SoDepthTabPlaneDragger::SHIFTALT ||
	key==SoDepthTabPlaneDragger::SHIFT )
	    state |= OD::ShiftButton;

    if ( key==SoDepthTabPlaneDragger::SHIFTCONTROLALT ||
	key==SoDepthTabPlaneDragger::SHIFTCONTROL ||
	key==SoDepthTabPlaneDragger::CONTROLALT ||
	key==SoDepthTabPlaneDragger::CONTROL )
	    state |= OD::ControlButton;

    if ( key==SoDepthTabPlaneDragger::SHIFTCONTROLALT ||
	key==SoDepthTabPlaneDragger::SHIFTALT ||
	key==SoDepthTabPlaneDragger::CONTROLALT ||
	key==SoDepthTabPlaneDragger::ALT )
	    state |= OD::AltButton;

    return (OD::ButtonState) state;
}



Coord3 DepthTabPlaneDragger::world2Dragger( const Coord3& world,
					    bool ispos ) const
{
    const Coord3 tpos = transform_ && ispos
	? transform_->transform(world) : world;

    if ( !dim_ )
	return Coord3( tpos.z, tpos.y, tpos.x );
    if ( dim_==1 )
	return Coord3( tpos.x, tpos.z, tpos.y );

    return tpos;
}


Coord3 DepthTabPlaneDragger::dragger2World( const Coord3& drag,
					    bool ispos ) const
{
    const Coord3 tpos = transform_ && ispos
	? transform_->transformBack(drag) : drag;
    if ( !dim_ )
	return Coord3( tpos.z, tpos.y, tpos.x );
    if ( dim_==1 )
	return Coord3( tpos.x, tpos.z, tpos.y );

    return tpos;
}

static SbVec3f startcenter_;

void DepthTabPlaneDragger::startCB( void* obj, SoDragger* sod )
{
    startcenter_ = ((SoDepthTabPlaneDragger*)sod)->translation.getValue();
    ((DepthTabPlaneDragger*)obj)->started.trigger();
}


void DepthTabPlaneDragger::motionCB( void* obj, SoDragger* sod )
{
    const TabletInfo* ti = TabletInfo::currentState();
    if ( ti && ti->maxPostPressDist()<5 )
	((SoDepthTabPlaneDragger*)sod)->translation.setValue( startcenter_ );
    else
	((DepthTabPlaneDragger*)obj)->motion.trigger();
}


void DepthTabPlaneDragger::valueChangedCB( void* obj, SoDragger* d )
{
    ((DepthTabPlaneDragger*)obj)->changed.trigger();
}


void DepthTabPlaneDragger::finishCB( void* obj, SoDragger* )
{
    ((DepthTabPlaneDragger*)obj)->finished.trigger();
}


void DepthTabPlaneDragger::showDraggerBorder( bool yn )
{
    if ( osgdragger_ )
    {
	const float borderopacity = yn ? 1.0 : 0.0;
	for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
	{
	    mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			     osgdragger_->getDragger(idx) );
	    if ( tpd )
	    {
		tpd->getTranslate2DDragger()->setColor(
					osg::Vec4(0.5,0.5,0.5,borderopacity) );
		tpd->getTranslate2DDragger()->setPickColor(
					osg::Vec4(1.0,1.0,1.0,borderopacity) );
	    }
	}
    }
}


bool DepthTabPlaneDragger::isDraggerBorderShown() const
{
    if ( osgdragger_ )
    {
	for ( int idx=osgdragger_->getNumDraggers()-1; idx>=0; idx-- )
	{
	    mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, tpd,
			     osgdragger_->getDragger(idx) );
	    if ( tpd )
		return tpd->getTranslate2DDragger()->getColor()[3];
	}
    }

    return false;
}


void DepthTabPlaneDragger::showPlane( bool yn )
{
    if ( osgdragger_ && osgdraggerplane_ )
	osgdraggerplane_->setValue( 0, yn );
}


bool DepthTabPlaneDragger::isPlaneShown() const
{ return osgdraggerplane_ && osgdraggerplane_->getValue(0); }


void DepthTabPlaneDragger::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    par.set( dimstr(), getDim() );

    const_cast<Coord3&>(centers_[dim_]) = center();
    for ( int idx=0; idx<3; idx++ )
    {
	BufferString str( centerstr() );
	str += idx;
	par.set( str, centers_[idx] );

	str = sizestr();
	str += idx;
	par.set( str, sizes_[idx] );
    }
}


int DepthTabPlaneDragger::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    for ( int idx=0; idx<3; idx++ )
    {
	BufferString str( centerstr() );
	str += idx;
	par.get( str, centers_[idx] );

	str = sizestr();
	str += idx;
	par.get( str, sizes_[idx] );
    }

    setSize( sizes_[dim_], false );
    setCenter( centers_[dim_], false );

    int dim = 0;
    par.get( dimstr(), dim );
    setDim( dim );

    return 1;
}


}; // namespace visBase
