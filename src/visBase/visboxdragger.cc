/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visboxdragger.h"

#include "vistransform.h"
#include "ranges.h"
#include "iopar.h"
#include "survinfo.h"

#include <osg/CullFace>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Version>

#include <osgGeo/TabBoxDragger>


mCreateFactoryEntry( visBase::BoxDragger );


namespace visBase
{


class BoxDraggerCallbackHandler: public osgManipulator::DraggerCallback
{

public: 
				BoxDraggerCallbackHandler( BoxDragger& dragger )
				    : dragger_( dragger )
				{}

    using			osgManipulator::DraggerCallback::receive;
    virtual bool		receive(const osgManipulator::MotionCommand&);

protected:

    void			adjustPolygonOffset(bool start);
    void			constrain(bool translated);

    BoxDragger&			dragger_;
    osg::Matrix			initialosgmatrix_;
    Coord3			initialcenter_;
};


bool BoxDraggerCallbackHandler::receive(
				    const osgManipulator::MotionCommand& cmd )
{
    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
    {
	initialosgmatrix_ = dragger_.osgboxdragger_->getMatrix();
	initialcenter_ = dragger_.center();
    }

    mDynamicCastGet( const osgManipulator::Scale1DCommand*, s1d, &cmd );
    mDynamicCastGet( const osgManipulator::Scale2DCommand*, s2d, &cmd );
    mDynamicCastGet( const osgManipulator::TranslateInPlaneCommand*,
		     translatedinplane, &cmd );

    if ( !s1d && !s2d && !translatedinplane )
    {
	dragger_.osgboxdragger_->setMatrix( initialosgmatrix_ );
	return true;
    }

    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
    {
	adjustPolygonOffset( true );
	dragger_.started.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::MOVE )
    {
	constrain( translatedinplane );
	dragger_.motion.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::FINISH )
    {
	adjustPolygonOffset( false );
	dragger_.finished.trigger();
	if ( initialosgmatrix_ != dragger_.osgboxdragger_->getMatrix() )
	    dragger_.changed.trigger();
    }

    return true;
}


void BoxDraggerCallbackHandler::adjustPolygonOffset( bool start )
{
    for ( int idx=dragger_.osgboxdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TabPlaneDragger*, tpd,
			 dragger_.osgboxdragger_->getDragger(idx) );
	if ( !tpd )
	    continue;

	for ( int idy=tpd->getNumDraggers()-1; idy>=0; idy-- )
	{
	    mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, dragger,
			     tpd->getDragger(idy) );
	    if ( !dragger )
		continue;

	    osg::StateSet* ss =
		    dragger->getTranslate2DDragger()->getOrCreateStateSet();

	    if ( !start )
		ss->removeAttribute( osg::StateAttribute::POLYGONOFFSET );
	    else if ( !ss->getAttribute(osg::StateAttribute::POLYGONOFFSET) )
	    {
		ss->setAttributeAndModes(
		    new osg::PolygonOffset(0.0,0.0),
		    osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );
	    }
	}
    }
}


void BoxDraggerCallbackHandler::constrain( bool translated )
{
    Coord3 scale = dragger_.width();
    Coord3 center = dragger_.center();

    for ( int dim=0; dim<3; dim++ )
    {
	if ( dragger_.spaceranges_[dim].width(false) > 0.0 )
	{
	    double diff = center[dim] - 0.5*scale[dim] -
			  dragger_.spaceranges_[dim].start;
	    if ( diff < 0.0 )
	    {
		center[dim] -= translated ? diff : 0.5*diff;
		if ( !translated )
		    scale[dim] += diff;
	    }

	    diff = center[dim] + 0.5*scale[dim] -
		   dragger_.spaceranges_[dim].stop;
	    if ( diff > 0.0 )
	    {
		center[dim] -= translated ? diff : 0.5*diff;
		if ( !translated )
		    scale[dim] -= diff;
	    }
	}

	if ( !translated && dragger_.widthranges_[dim].width(false)>0.0 )
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


//=============================================================================


BoxDragger::BoxDragger()
    : started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , osgcallbackhandler_( 0 )
    , osgboxdragger_( setOsgNode( new osgGeo::TabBoxDragger(12.0) ) )
{
    osgboxdragger_->setupDefaultGeometry();
    osgboxdragger_->setHandleEvents( true );

    osgcallbackhandler_ = new BoxDraggerCallbackHandler( *this );
    osgboxdragger_->addDraggerCallback( osgcallbackhandler_ );

    showScaleTabs( true );

    osgboxdragger_->getOrCreateStateSet()->setAttributeAndModes(
		    new osg::PolygonOffset(-1.0,-1.0),
		    osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );

#if OSG_MIN_VERSION_REQUIRED(3,1,3)
    osgboxdragger_->setIntersectionMask( cIntersectionTraversalMask() );
    osgboxdragger_->setActivationMouseButtonMask(
				osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON |
				osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON );
#endif

    osgboxdragger_->getOrCreateStateSet()->setRenderingHint(
					    osg::StateSet::TRANSPARENT_BIN );

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    osgdraggerbox_ = new osg::ShapeDrawable;
    osgdraggerbox_->setShape( new osg::Box(osg::Vec3(0.0,0.0,0.0), 1.0) );

    geode->addDrawable( osgdraggerbox_ );
    geode->getOrCreateStateSet()->setMode( GL_BLEND, osg::StateAttribute::ON );
    geode->getStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    geode->getStateSet()->setAttributeAndModes(
		    new osg::PolygonOffset(1.0,1.0),
		    osg::StateAttribute::PROTECTED | osg::StateAttribute::ON );
    geode->getStateSet()->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
    geode->setNodeMask( geode->getNodeMask() &
	    		~visBase::cIntersectionTraversalMask() );

    osg::ref_ptr<osg::CullFace> cullface = new osg::CullFace;
    cullface->setMode( osg::CullFace::FRONT );
    geode->getStateSet()->setAttributeAndModes( cullface,
	    					osg::StateAttribute::ON );

    osgboxdragger_->addChild( geode );

    showDraggerBorder( true );
}


BoxDragger::~BoxDragger()
{
    osgboxdragger_->removeDraggerCallback( osgcallbackhandler_ );
}


void BoxDragger::setOsgMatrix( const Coord3& worldscale,
			       const Coord3& worldtrans )
{
    osg::Vec3d scale, trans;
    mVisTrans::transformDir( transform_, worldscale, scale );
    mVisTrans::transform( transform_, worldtrans, trans );

    osg::Matrix mat;
    mat *= osg::Matrix::scale( scale );
    mat *= osg::Matrix::translate( trans );
    osgboxdragger_->setMatrix( mat );
}


void BoxDragger::setBoxTransparency( float transparency )
{
    osgdraggerbox_->setColor( osg::Vec4(0.7,0.7,0.7,1.0-transparency) );
}


void BoxDragger::showScaleTabs( bool yn )
{
    const float tabopacity = yn ? 1.0 : 0.0;

    for ( int idx=osgboxdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TabPlaneDragger*, tpd,
			 osgboxdragger_->getDragger(idx) );
	if ( !tpd )
	    continue;

	for ( int idy=tpd->getNumDraggers()-1; idy>=0; idy-- )
	{
	    osgManipulator::Dragger* dragger = tpd->getDragger( idy );
	    mDynamicCastGet( osgManipulator::Scale1DDragger*, s1dd, dragger );
	    if ( s1dd )
	    {
		s1dd->setColor( osg::Vec4(0.0,0.7,0.0,tabopacity) );
		s1dd->setPickColor( osg::Vec4(0.0,1.0,0.0,tabopacity) );
	    }
	    mDynamicCastGet( osgManipulator::Scale2DDragger*, s2dd, dragger );
	    if ( s2dd )
	    {
		s2dd->setColor( osg::Vec4(0.0,0.7,0.0,tabopacity) );
		s2dd->setPickColor( osg::Vec4(0.0,1.0,0.0,tabopacity) );
	    }
	}
    }

}


void BoxDragger::setCenter( const Coord3& pos )
{
    setOsgMatrix( width(), pos );
}


Coord3 BoxDragger::center() const
{
    Coord3 trans;
    mVisTrans::transformBack( transform_,
			      osgboxdragger_->getMatrix().getTrans(),
			      trans );
    return trans;
}


void BoxDragger::setWidth( const Coord3& scale )
{
    setOsgMatrix( scale, center() );
}


Coord3 BoxDragger::width() const
{
    Coord3 scale;
    mVisTrans::transformBackDir( transform_,
				 osgboxdragger_->getMatrix().getScale(),
				 scale );

    scale.x = fabs(scale.x); scale.y = fabs(scale.y); scale.z = fabs(scale.z);
    return scale;
}


void BoxDragger::setSpaceLimits( const Interval<float>& x,
				 const Interval<float>& y,
				 const Interval<float>& z )
{
    spaceranges_[0] = x; spaceranges_[1] = y; spaceranges_[2] = z;

}


void BoxDragger::setWidthLimits( const Interval<float>& x,
				 const Interval<float>& y,
				 const Interval<float>& z )
{
    widthranges_[0] = x; widthranges_[1] = y; widthranges_[2] = z;
}


void BoxDragger::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transform_ == nt )
	return;

    const Coord3 oldcenter = center();
    const Coord3 oldwidth = width();

    transform_ = nt;

    setWidth( oldwidth );
    setCenter( oldcenter );
}


const mVisTrans* BoxDragger::getDisplayTransformation() const
{
    return transform_;
}


void BoxDragger::showDraggerBorder( bool yn )
{
    const float borderopacity = yn ? 1.0 : 0.0;
    for ( int idx=osgboxdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TabPlaneDragger*, tpd,
			 osgboxdragger_->getDragger(idx) );
	if ( !tpd )
	    continue;

	for ( int idy=tpd->getNumDraggers()-1; idy>=0; idy-- )
	{
	    mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, dragger,
			     tpd->getDragger(idy) );
	    if ( !dragger )
		continue;

	    const osg::Vec4 col( 0.5, 0.5, 0.5, borderopacity );
	    if ( col != dragger->getTranslate2DDragger()->getColor() )
		dragger->getTranslate2DDragger()->setColor( col );

	    const osg::Vec4 pickcol( 1.0, 1.0, 1.0, borderopacity );
	    if ( pickcol != dragger->getTranslate2DDragger()->getPickColor() )
		dragger->getTranslate2DDragger()->setPickColor( pickcol );
	}
    }
}


bool BoxDragger::isDraggerBorderShown() const
{
    for ( int idx=osgboxdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TabPlaneDragger*, tpd,
			 osgboxdragger_->getDragger(idx) );
	if ( !tpd )
	    continue;

	for ( int idy=tpd->getNumDraggers()-1; idy>=0; idy-- )
	{
	    mDynamicCastGet( osgManipulator::TranslatePlaneDragger*, dragger,
		    	     tpd->getDragger(idy) );
	    if ( dragger )
		return dragger->getTranslate2DDragger()->getColor()[3];
	}
    }

    return false;
}


}; // namespace visBase
