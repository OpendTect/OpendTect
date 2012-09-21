/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "visboxdragger.h"
#include "ranges.h"
#include "iopar.h"
#include "survinfo.h"

#include <Inventor/draggers/SoTabBoxDragger.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoShapeHints.h>

#include <osgManipulator/TabBoxDragger>
#include <osg/Switch>

mCreateFactoryEntry( visBase::BoxDragger );


namespace visBase
{


static void setOsgMatrix( osgManipulator::TabBoxDragger& osgdragger,
			  const osg::Vec3& scale, const osg::Vec3& trans )
{
    osg::Matrix mat;
    mat *= osg::Matrix::scale( scale );
    mat *= osg::Matrix::translate( trans );
    osgdragger.setMatrix( mat );
}


class BoxDraggerCallbackHandler: public osgManipulator::DraggerCallback
{

public: 
				BoxDraggerCallbackHandler( BoxDragger& dragger )
				    : dragger_( dragger )
				{}

    using			osgManipulator::DraggerCallback::receive;
    virtual bool		receive(const osgManipulator::MotionCommand&);

protected:

    void			constrain();

    BoxDragger&			dragger_;
    osg::Matrix			startmatrix_;
};


bool BoxDraggerCallbackHandler::receive(
				    const osgManipulator::MotionCommand& cmd )
{
    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
	startmatrix_ = dragger_.osgboxdragger_->getMatrix();

    mDynamicCastGet( const osgManipulator::Scale1DCommand*, s1d, &cmd );
    mDynamicCastGet( const osgManipulator::Scale2DCommand*, s2d, &cmd );
    mDynamicCastGet( const osgManipulator::TranslateInPlaneCommand*,
		     translatedinplane, &cmd );

    if ( !s1d && !s2d && !translatedinplane )
    {
	dragger_.osgboxdragger_->setMatrix( startmatrix_ );
	return true;
    }

    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
    {
	dragger_.started.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::MOVE )
    {
	constrain();
	dragger_.motion.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::FINISH )
    {
	dragger_.finished.trigger();
	if ( startmatrix_ != dragger_.osgboxdragger_->getMatrix() )
	    dragger_.changed.trigger();
    }

    return true;
}


void BoxDraggerCallbackHandler::constrain()
{
    osg::Vec3 scale = dragger_.osgboxdragger_->getMatrix().getScale();
    osg::Vec3 center = dragger_.osgboxdragger_->getMatrix().getTrans();

    for ( int dim=0; dim<3; dim++ )
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

    setOsgMatrix( *dragger_.osgboxdragger_, scale, center );
}


//=============================================================================


BoxDragger::BoxDragger()
    : started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , onoff_( new SoSwitch )
    , boxdragger_( new SoTabBoxDragger )
    , xinterval_( 0 )
    , yinterval_( 0 )
    , zinterval_( 0 )
    , selectable_( false )
    , boxmaterial_ ( new SoMaterial )
    , osgboxdragger_( 0 )
    , osgdraggerroot_( 0 )
    , osgcallbackhandler_( 0 )
{
    if ( doOsg() )
	initOsgDragger();

    boxmaterial_->ref();
    onoff_->addChild( boxdragger_ );
    onoff_->ref();
    boxdragger_->addStartCallback(
	    BoxDragger::startCB, this );
    boxdragger_->addMotionCallback(
	    BoxDragger::motionCB, this );
    boxdragger_->addValueChangedCallback(
	    BoxDragger::valueChangedCB, this );
    boxdragger_->addFinishCallback(
	    BoxDragger::finishCB, this );

    setOwnShapeHints();

#ifdef __debug__
    SoSeparator* boxsep = new SoSeparator;
    boxsep->ref();

    boxsep->addChild( boxmaterial_ );
    
    SoPickStyle* boxstyle = new SoPickStyle;
    boxsep->addChild( boxstyle );
    boxstyle->style = SoPickStyle::UNPICKABLE;

    SoShapeHints* hints = new SoShapeHints;
    boxsep->addChild( hints );
    hints->vertexOrdering = SI().isClockWise()
	? SoShapeHints::COUNTERCLOCKWISE : SoShapeHints::CLOCKWISE;

    SoCoordinate3* coords = new SoCoordinate3;
    boxsep->addChild( coords );
    coords->point.set1Value( 0, -1, -1, -1 );
    coords->point.set1Value( 1, -1, -1,  1 );
    coords->point.set1Value( 2, -1,  1, -1 );
    coords->point.set1Value( 3, -1,  1,  1 );
    coords->point.set1Value( 4,  1, -1, -1 );
    coords->point.set1Value( 5,  1, -1,  1 );
    coords->point.set1Value( 6,  1,  1, -1 );
    coords->point.set1Value( 7,  1,  1,  1 );

    SoNormalBinding* nb = new SoNormalBinding;
    boxsep->addChild( nb );
    nb->value = SoNormalBinding::PER_FACE;

    SoIndexedTriangleStripSet* strip = new SoIndexedTriangleStripSet;
    boxsep->addChild( strip );
    const int tricoordindices[] =
	{ 0, 1, 2, 3, 6, 7, 4, 5, 0, 1, -1, 0, 2, 4, 6, -1, 1, 5, 3, 7 };
    strip->coordIndex.setValues( 0, 20, tricoordindices );

    SoMaterial* linematerial = new SoMaterial;
    boxsep->addChild( linematerial );

    const int linecoordincices[] =
	{ 0, 1, 3, 2, 0, 4, 6, 2, -1, 4, 5, 7, 6, -1, 3, 7, -1, 1, 5 };
    SoIndexedLineSet* lines = new SoIndexedLineSet;
    boxsep->addChild( lines );
    lines->coordIndex.setValues( 0, 19, linecoordincices );

    boxdragger_->setPart( "boxGeom", boxsep );
    boxsep->unref();
#endif
}


BoxDragger::~BoxDragger()
{
    if ( osgdraggerroot_ && osgboxdragger_ )
    {
	osgboxdragger_->removeDraggerCallback( osgcallbackhandler_ );
	osgdraggerroot_->unref();
    }

    boxdragger_->removeStartCallback(
	    BoxDragger::startCB, this );
    boxdragger_->removeMotionCallback(
	    BoxDragger::motionCB, this );
    boxdragger_->removeValueChangedCallback(
	    BoxDragger::valueChangedCB, this );
    boxdragger_->removeFinishCallback(
	    BoxDragger::finishCB, this );

    onoff_->unref();
    boxmaterial_->unref();

    delete xinterval_;
    delete yinterval_;
    delete zinterval_;
}


void BoxDragger::initOsgDragger()
{
    if ( !doOsg() || osgboxdragger_ )
	return;

    osgdraggerroot_ = new osg::Switch();
    osgdraggerroot_->ref();

    osgboxdragger_ = new osgManipulator::TabBoxDragger();
    osgdraggerroot_->addChild( osgboxdragger_ );

    osgboxdragger_->setupDefaultGeometry();
    osgboxdragger_->setHandleEvents( true );
    setBoxTransparency( 0.0 );

    osgcallbackhandler_ = new BoxDraggerCallbackHandler( *this );
}


void BoxDragger::setBoxTransparency( float f )
{
    boxmaterial_->transparency.setValue( f );

    if ( osgboxdragger_ )
	osgboxdragger_->setPlaneColor( osg::Vec4(0.7,0.7,0.7,1.0-f) );
}


void BoxDragger::setOwnShapeHints()
{
    SoShapeHints* myHints = new SoShapeHints;
    myHints->shapeType = SoShapeHints::SOLID;
    myHints->vertexOrdering = SI().isClockWise()
	? SoShapeHints::COUNTERCLOCKWISE : SoShapeHints::CLOCKWISE;
    
    SoDragger* child;
    const char* tabstr( "tabPlane" );
    for ( int i = 1; i <= 6; i++ )
    {
	BufferString str( tabstr ); str += i;
	child = (SoDragger*)boxdragger_->getPart( str.buf(), false );
	child->setPart( "scaleTabHints", myHints );
    }
}


void BoxDragger::setCenter( const Coord3& pos )
{
    boxdragger_->translation.setValue( (float) pos.x, 
				    (float) pos.y, (float) pos.z );
    prevcenter_ = pos;

    if ( osgboxdragger_ )
    {
	setOsgMatrix( *osgboxdragger_, osgboxdragger_->getMatrix().getScale(),
		      osg::Vec3(pos.x,pos.y,pos.z) );
    }
}


Coord3 BoxDragger::center() const
{
    if ( osgboxdragger_ )
    {
	osg::Vec3 dragcenter = osgboxdragger_->getMatrix().getTrans();
	return Coord3( dragcenter[0], dragcenter[1], dragcenter[2] );
    }

    SbVec3f pos = boxdragger_->translation.getValue();
    return Coord3( pos[0], pos[1], pos[2] );
}


void BoxDragger::setWidth( const Coord3& pos )
{
    boxdragger_->scaleFactor.setValue( (float) pos.x/2, 
			    (float) pos.y/2, (float) pos.z/2 );
    prevwidth_ = pos;

    if ( osgboxdragger_ )
    {
	setOsgMatrix( *osgboxdragger_, osg::Vec3f(pos.x,pos.y,pos.z),	
		      osgboxdragger_->getMatrix().getTrans() );
    }
}


Coord3 BoxDragger::width() const
{
    if ( osgboxdragger_ )
    {
	osg::Vec3 boxwidth = osgboxdragger_->getMatrix().getScale();
	return Coord3( boxwidth[0], boxwidth[1], boxwidth[2] );
    }

    SbVec3f pos = boxdragger_->scaleFactor.getValue();
    return Coord3( pos[0]*2, pos[1]*2, pos[2]*2 );
}


void BoxDragger::setSpaceLimits( const Interval<float>& x,
				 const Interval<float>& y,
				 const Interval<float>& z )
{
    if ( !xinterval_ )
    {
	xinterval_ = new Interval<float>(x);
	yinterval_ = new Interval<float>(y);
	zinterval_ = new Interval<float>(z);
	return;
    }

    *xinterval_ = x;
    *yinterval_ = y;
    *zinterval_ = z;

    if ( doOsg() )
    {
	spaceranges_[0] = x; spaceranges_[1] = y; spaceranges_[2] = z;
    }
}


void BoxDragger::setWidthLimits( const Interval<float>& x,
				 const Interval<float>& y,
				 const Interval<float>& z )
{
    widthranges_[0] = x; widthranges_[1] = y; widthranges_[2] = z;
}


void BoxDragger::turnOn( bool yn )
{
    onoff_->whichChild = yn ? 0 : SO_SWITCH_NONE;

    if ( osgdraggerroot_ )
	osgdraggerroot_->setValue( 0, yn );
}


bool BoxDragger::isOn() const
{
    if ( osgdraggerroot_ )
	return osgdraggerroot_->getValue(0);

    return !onoff_->whichChild.getValue();
}


SoNode* BoxDragger::gtInvntrNode()
{ return onoff_; }


osg::Node* BoxDragger::gtOsgNode()
{ return osgdraggerroot_; }


void BoxDragger::startCB( void* obj, SoDragger* )
{
    BoxDragger* thisp = (BoxDragger*)obj;
    thisp->prevcenter_ = thisp->center();
    thisp->prevwidth_ = thisp->width();
    thisp->started.trigger();
}


void BoxDragger::motionCB( void* obj, SoDragger* )
{
    ( (BoxDragger*)obj )->motion.trigger();
}

#define mCheckDim(dim)\
if ( thisp->dim##interval_ )\
{\
    if ( !thisp->dim##interval_->includes(center.dim-width.dim/2,true) || \
	 !thisp->dim##interval_->includes(center.dim+width.dim/2,true))\
    {\
	if ( constantwidth ) center.dim = thisp->prevcenter_.dim; \
	else \
	{ \
	    width.dim = thisp->prevwidth_.dim; \
	    center.dim = thisp->prevcenter_.dim; \
	} \
	change = true; \
    }\
}

void BoxDragger::valueChangedCB( void* obj, SoDragger* )
{
    BoxDragger* thisp = (BoxDragger*)obj;

    Coord3 center = thisp->center();
    Coord3 width = thisp->width();

    const bool constantwidth =
	mIsEqualRel(width.x,thisp->prevwidth_.x,1e-6) &&
	mIsEqualRel(width.y,thisp->prevwidth_.y,1e-6) &&
	mIsEqualRel(width.z,thisp->prevwidth_.z,1e-6);

    if  ( constantwidth && center==thisp->prevcenter_ )
	return;

    bool change = false;
    mCheckDim(x);
    mCheckDim(y);
    mCheckDim(z);

    if ( change )
    {
	thisp->setCenter( center );
	thisp->setWidth(  width );
    }

    thisp->prevcenter_ = center;
    thisp->prevwidth_ = width;

    ( (BoxDragger*)obj )->changed.trigger();
}


void BoxDragger::finishCB( void* obj, SoDragger* )
{
    ( (BoxDragger*)obj )->finished.trigger();
}


}; // namespace visBase
