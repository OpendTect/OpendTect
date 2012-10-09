/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

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

mCreateFactoryEntry( visBase::BoxDragger );

namespace visBase
{

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
{
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

    const int trinormindices[] =
	{ 0, 1, 2, 3, 6, 7, 4, 5, 0, 1, -1, 0, 2, 4, 6, -1, 1, 5, 3, 7 };

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


void BoxDragger::setBoxTransparency( float f )
{
    boxmaterial_->transparency.setValue( f );
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
    boxdragger_->translation.setValue( pos.x, pos.y, pos.z );
    prevcenter_ = pos;
}


Coord3 BoxDragger::center() const
{
    SbVec3f pos = boxdragger_->translation.getValue();
    return Coord3( pos[0], pos[1], pos[2] );
}


void BoxDragger::setWidth( const Coord3& pos )
{
    boxdragger_->scaleFactor.setValue( pos.x/2, pos.y/2, pos.z/2 );
    prevwidth_ = pos;
}


Coord3 BoxDragger::width() const
{
    SbVec3f pos = boxdragger_->scaleFactor.getValue();
    return Coord3( pos[0]*2, pos[1]*2, pos[2]*2 );
}


void BoxDragger::setSpaceLimits( const Interval<float>& x,
					  const Interval<float>& y,
					  const Interval<float>& z)
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
}


void BoxDragger::turnOn( bool yn )
{
    onoff_->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool BoxDragger::isOn() const
{
    return !onoff_->whichChild.getValue();
}


SoNode* BoxDragger::gtInvntrNode()
{ return onoff_; }


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
