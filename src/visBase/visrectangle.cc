/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visrectangle.cc,v 1.7 2002-02-28 07:51:08 kristofer Exp $";

#include "visrectangle.h"
#include "visscene.h"
#include "visselman.h"

#include "Inventor/nodes/SoScale.h"
#include "Inventor/nodes/SoTranslation.h"
#include "Inventor/nodes/SoRotation.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoFaceSet.h"
#include "Inventor/nodes/SoNormal.h"
#include "Inventor/nodes/SoNormalBinding.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoDrawStyle.h"
#include "Inventor/nodes/SoMaterial.h"
#include "Inventor/nodes/SoShapeHints.h"

#include "Inventor/draggers/SoTabPlaneDragger.h"
#include "Inventor/draggers/SoTranslate1Dragger.h"

#include "Inventor/actions/SoGetMatrixAction.h"


visBase::RectangleDragger::RectangleDragger()
    : started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , root( new SoSeparator )
    , manipzdraggertop( new SoTranslate1Dragger )
    , manipzdraggerright( new SoTranslate1Dragger )
    , manipzdraggerbottom( new SoTranslate1Dragger )
    , manipzdraggerleft( new SoTranslate1Dragger )
    , manipxydragger( new SoTabPlaneDragger )
    , zdraggerscale( new SoScale )
    , allowcb( true )
{
    SoSeparator* zmanipsep = new SoSeparator;
    root->addChild( zmanipsep );

    zmanipsep->addChild( zdraggerscale );

    SoRotation* rot = new SoRotation;
    zmanipsep->addChild( rot );
    rot->rotation.setValue( SbVec3f( 0,1,0), -M_PI/2 );

    zmanipsep->addChild( manipzdraggertop );
    manipzdraggertop->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggertop->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggertop->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggertop->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    zmanipsep->addChild( manipzdraggerright );
    manipzdraggerright->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerright->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerright->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerright->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    zmanipsep->addChild( manipzdraggerbottom );
    manipzdraggerbottom->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerbottom->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerbottom->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerbottom->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    zmanipsep->addChild( manipzdraggerleft );
    manipzdraggerleft->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerleft->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerleft->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerleft->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    root->addChild( manipxydragger );
    manipxydragger->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipxydragger->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipxydragger->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipxydragger->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    syncronizeDraggers();
}


visBase::RectangleDragger::~RectangleDragger()
{
    manipzdraggertop->removeStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggertop->removeMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggertop->removeValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggertop->removeFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    manipzdraggerright->removeStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerright->removeMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerright->removeValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerright->removeFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    manipzdraggerbottom->removeStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerbottom->removeMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerbottom->removeValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerbottom->removeFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    manipzdraggerleft->removeStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerleft->removeMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerleft->removeValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerleft->removeFinishCallback(
	    visBase::RectangleDragger::finishCB, this );
}


void visBase::RectangleDragger::setCenter( float x, float y, float z )
{
    manipxydragger->translation.setValue( x, y, z );

    float xd = manipzdraggertop->translation.getValue()[2] /
	       zdraggerscale->scaleFactor.getValue()[0];
    float yd = manipzdraggertop->translation.getValue()[1] /
	       zdraggerscale->scaleFactor.getValue()[0];

    z /= zdraggerscale->scaleFactor.getValue()[2];

    manipzdraggertop->translation.setValue( z, yd, xd );

    xd = manipzdraggerright->translation.getValue()[2];
    yd = manipzdraggerright->translation.getValue()[1];
    manipzdraggerright->translation.setValue( z, yd, xd );

    xd = manipzdraggerbottom->translation.getValue()[2];
    yd = manipzdraggerbottom->translation.getValue()[1];
    manipzdraggerbottom->translation.setValue( z, yd, xd );

    xd = manipzdraggerleft->translation.getValue()[2];
    yd = manipzdraggerleft->translation.getValue()[1];
    manipzdraggerleft->translation.setValue( z, yd, xd );

    syncronizeDraggers();
}


float visBase::RectangleDragger::center( int dim ) const
{
    return manipxydragger->translation.getValue()[dim];
}


void visBase::RectangleDragger::setScale( float x, float y )
{
    manipxydragger->scaleFactor.setValue( x, y, 1 );
}



float visBase::RectangleDragger::scale( int dim ) const
{
    return manipxydragger->scaleFactor.getValue()[dim];
}


void visBase::RectangleDragger::setDraggerSize( float w, float h, float d )
{
    zdraggerscale->scaleFactor.setValue( w, h, d );
    syncronizeDraggers();
}


SoNode* visBase::RectangleDragger::getData()
{ return root; }



void visBase::RectangleDragger::syncronizeDraggers()
{
    SbVec3f xyscale(1,1,1);

    float x = manipxydragger->translation.getValue()[0] * xyscale[0];
    float y = manipxydragger->translation.getValue()[1] * xyscale[1];

    SbVec3f zdragscale( zdraggerscale->scaleFactor.getValue() );

    float z = manipzdraggertop->translation.getValue()[0] * zdragscale[2];

    float xscale = manipxydragger->scaleFactor.getValue()[0] * xyscale[0];
    float yscale = manipxydragger->scaleFactor.getValue()[1] * xyscale[1];

    bool allowcb_bak = allowcb;
    allowcb = false;

    if ( manipzdraggertop )
    {
	manipzdraggertop->translation.setValue( z/zdragscale[2],
					    (y + 1.1 * yscale)/zdragscale[1],
					    (-x)/zdragscale[0] );
	manipzdraggerright->translation.setValue( z/zdragscale[2],
					    (y)/zdragscale[1],
					    (-x-1.1*xscale)/zdragscale[0] );
	manipzdraggerbottom->translation.setValue( z/zdragscale[2],
					    (y - 1.1 * yscale)/zdragscale[1],
					    (-x)/zdragscale[0] );
	manipzdraggerleft->translation.setValue( z/zdragscale[2],
					    (y)/zdragscale[1],
					    (-x+1.1*xscale)/zdragscale[0] );
    }

    if ( manipxydragger )
    {
	manipxydragger->translation.setValue( x / xyscale[0],
					y / xyscale[1], z / xyscale[2] );
    }

    allowcb = allowcb_bak;
}


void visBase::RectangleDragger::draggerHasMoved( SoDragger* d )
{
    if ( !allowcb ) return;

    SoTranslate1Dragger* zd = dynamic_cast<SoTranslate1Dragger*>( d );
    if ( zd )
    {
	if ( zd!=manipzdraggertop )
	{
	    float x = manipzdraggertop->translation.getValue()[2];
	    float y = manipzdraggertop->translation.getValue()[1];
	    float z = zd->translation.getValue()[0];

	    manipzdraggertop->translation.setValue( z, y, x );
	}

	if ( zd!=manipzdraggerright )
	{
	    float x = manipzdraggerright->translation.getValue()[2];
	    float y = manipzdraggerright->translation.getValue()[1];
	    float z = zd->translation.getValue()[0];

	    manipzdraggerright->translation.setValue( z, y, x );
	}

	if ( zd!=manipzdraggerbottom )
	{
	    float x = manipzdraggerbottom->translation.getValue()[2];
	    float y = manipzdraggerbottom->translation.getValue()[1];
	    float z = zd->translation.getValue()[0];

	    manipzdraggerbottom->translation.setValue( z, y, x );
	}

	if ( zd!=manipzdraggerleft )
	{
	    float x = manipzdraggerleft->translation.getValue()[2];
	    float y = manipzdraggerleft->translation.getValue()[1];
	    float z = zd->translation.getValue()[0];

	    manipzdraggerleft->translation.setValue( z, y, x );
	}
    }

    syncronizeDraggers();
}


void visBase::RectangleDragger::startCB(void* obj, SoDragger* )
{
    ((visBase::RectangleDragger*) obj)->started.trigger();
}


void visBase::RectangleDragger::motionCB(void* obj, SoDragger* )
{
    ((visBase::RectangleDragger*) obj)->motion.trigger();
}


void visBase::RectangleDragger::valueChangedCB(void* obj, SoDragger* d )
{
    ((visBase::RectangleDragger*) obj)->draggerHasMoved( d );
    ((visBase::RectangleDragger*) obj)->changed.trigger();
}


void visBase::RectangleDragger::finishCB(void* obj, SoDragger* )
{
    ((visBase::RectangleDragger*) obj)->finished.trigger();
}


visBase::Rectangle::Rectangle(Scene& scene_, bool usermanip)
    : VisualObjectImpl( scene_ )
    , origotrans( new SoTranslation )
    , orientationrot( new SoRotation )
    , orientation_( visBase::Rectangle::XY )
    , localorigotrans( new SoTranslation )
    , localscale( new SoScale )
    , widthscale( new SoScale )
    , planesep( new SoSeparator )
    , planewrapper( new SoFaceSet )
    , manipswitch( 0 )
    , maniprectswitch( 0 )
    , dragger( usermanip ? new RectangleDragger : 0 )
    , snap( false )
    , xrange( -mUndefValue, mUndefValue, mUndefValue )
    , yrange( -mUndefValue, mUndefValue, mUndefValue )
    , zrange( -mUndefValue, mUndefValue, mUndefValue )
    , wxrange( 1, mUndefValue )
    , wyrange( 1, mUndefValue )
{ 
    root->addChild( origotrans );
    root->addChild( orientationrot );
    root->addChild( widthscale );
    root->addChild( localorigotrans );
    localorigotrans->translation.setValue( 0.5, 0.5, 0 );

    root->addChild( localscale );
    float localwidth = 2;
    localscale->scaleFactor.setValue(	1.0/localwidth,
	    				1.0/localwidth,
					1.0/localwidth );

    SoCoordinate3* coords = new SoCoordinate3;
    root->addChild( coords );
    float hlocalwidth = localwidth / 2;
    coords->point.set1Value( 0, -hlocalwidth, -hlocalwidth, 0 );
    coords->point.set1Value( 1, -hlocalwidth,  hlocalwidth, 0 );
    coords->point.set1Value( 2,  hlocalwidth,  hlocalwidth, 0 );
    coords->point.set1Value( 3,  hlocalwidth, -hlocalwidth, 0 );
    coords->point.set1Value( 4, -hlocalwidth, -hlocalwidth, 0 );

    SoNormal* normals = new SoNormal;
    root->addChild( normals );
    normals->vector.setValue( 0, 0, 1 );

    SoShapeHints* shapehint = new SoShapeHints;
    root->addChild( shapehint );

    shapehint->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    shapehint->vertexOrdering = SoShapeHints::CLOCKWISE;

    SoNormalBinding* nbind = new SoNormalBinding;
    root->addChild( nbind );
    nbind->value = SoNormalBinding::PER_FACE;

    root->addChild( planesep );
    planesep->addChild( planewrapper.getData() );
    ((SoFaceSet*)planewrapper.getData())->numVertices.set1Value(0, 5);
    scene.selMan().regSelObject( *this, planewrapper );

    if ( dragger )
    {
	dragger->changed.notify(
			mCB(this, Rectangle, moveManipRectangletoDragger ));
	dragger->finished.notify( mCB(this, Rectangle, moveDraggertoManipRect));

	// Manip switch & separator
	manipswitch = new SoSwitch;
	root->addChild( manipswitch );

	SoSeparator* manipsep = new SoSeparator;
	manipswitch->addChild( manipsep );
	manipswitch->whichChild = SO_SWITCH_NONE;

	manipsep->addChild( dragger->getData() );

	// Manip rectangle
	maniprecttrans = new SoTranslation;
	manipsep->addChild( maniprecttrans );

	maniprectscale = new SoScale;
	manipsep->addChild( maniprectscale );

	SoMaterial* maniprectmaterial = new SoMaterial;
	manipsep->addChild( maniprectmaterial );
	maniprectmaterial->transparency.setValue( 0.5 );
	maniprectswitch = new SoSwitch;
	maniprectswitch->addChild( planewrapper.getData() );
	maniprectswitch->whichChild = SO_SWITCH_NONE;
	manipsep->addChild( maniprectswitch );
    }
}


visBase::Rectangle::~Rectangle()
{
    scene.selMan().unRegSelObject( *this );
    delete dragger;
}


void visBase::Rectangle::setOrigo( float x, float y, float z )
{
    origotrans->translation.setValue( x, y, z );
}


float visBase::Rectangle::origo( int n ) const
{
    return origotrans->translation.getValue()[n];
}


float visBase::Rectangle::manipOrigo( int dim ) const
{
    SbVec3f centerpos( maniprecttrans->translation.getValue());
    SbVec3f scale = maniprectscale->scaleFactor.getValue();

    SbVec3f origopos;

    float res;

    switch ( orientation_ )
    {
    case XY:
	if ( dim==0 ) res = getStartPos(0, centerpos[0], scale[0] );
	else if ( dim==1 ) res = getStartPos(1, centerpos[1], scale[1] );
	else res = getStartPos(2, centerpos[2], 0 );
    break;
    case XZ:
    	if ( dim==0 ) res = getStartPos(0, centerpos[0], scale[0] );
	else if ( dim==1 ) res = getStartPos(2, -centerpos[2], 0 );
	else res = getStartPos(1, centerpos[1], scale[1]);
    break;
    case YZ:
    	if ( dim==0 ) res = getStartPos(2, -centerpos[2], 0 );
	else if ( dim==1 ) res = getStartPos(1, centerpos[1], scale[1] );
	else res = getStartPos(0, centerpos[0], scale[0]);
    }

    return res;
}


void visBase::Rectangle::setWidth( float x, float y )
{
    widthscale->scaleFactor.setValue( x, y, 1 );
}


float visBase::Rectangle::width( int n ) const
{
    return getWidth( n, maniprectscale->scaleFactor.getValue()[n] );
}


void visBase::Rectangle::setOrientation( visBase::Rectangle::Orientation o )
{
    switch ( o )
    {
    case XY:
	orientationrot->rotation.setValue( SbVec3f(1,0,0), 0 );
    break;
    case XZ:
	orientationrot->rotation.setValue( SbVec3f(1,0,0), M_PI/2 );
    break;
    case YZ:
	orientationrot->rotation.setValue( SbVec3f(0,1,0), -M_PI/2 );
    break;
    }

    orientation_ = o;
}


void visBase::Rectangle::setRange( int dim, const StepInterval<float>& range )
{
    if ( dim==0 ) xrange = range;
    else if ( dim==1 ) yrange = range;
    else if ( dim==2 ) zrange = range;
}


void visBase::Rectangle::setWidthRange( int dim, const Interval<float>& range )
{
    if ( dim==0 ) wxrange = range;
    else if ( dim==1 ) wyrange = range;
}


void visBase::Rectangle::displayDraggers(bool on)
{
    if ( manipswitch ) manipswitch->whichChild = on ? 0 : SO_SWITCH_NONE;
}


void visBase::Rectangle::moveManipRectangletoDragger(CallBacker*)
{
    if ( !dragger ) return;

    float x = maniprecttrans->translation.getValue()[0];
    float y = maniprecttrans->translation.getValue()[1];
    float z = maniprecttrans->translation.getValue()[2];

    float xscale = maniprectscale->scaleFactor.getValue()[0];
    float yscale = maniprectscale->scaleFactor.getValue()[1];

    float newx = dragger->center(0);
    float newy = dragger->center(1);
    float newz = dragger->center(2);

    float newxscale = dragger->scale(0);
    float newyscale = dragger->scale(1);

    float startpos = snapPos( 0, getStartPos(0, newx, newxscale ));
    float stoppos = snapPos( 0, getStopPos( 0, newx, newxscale ) );
    if ( xrange.includes( startpos ) && xrange.includes( stoppos ) &&
	    wxrange.includes( stoppos-startpos))
    {
	x = getCenterCoord( 0, startpos, stoppos-startpos );
	xscale= getScale( 0, stoppos-startpos);
    }
    

    startpos = snapPos( 1, getStartPos(1, newy, newyscale ));
    stoppos = snapPos( 1, getStopPos( 1, newy, newyscale ));
    if ( yrange.includes( startpos ) && yrange.includes( stoppos ) &&
	    wxrange.includes( stoppos-startpos))
    {
	y = getCenterCoord( 1, startpos, stoppos-startpos );
	yscale= getScale( 1, stoppos-startpos);
    }
    

    startpos = snapPos(2, getStartPos( 2, orientation_!=XY ? -newz : newz, 0));
    if ( zrange.includes( startpos ) )
    {
	z = orientation_ != XY ? -getCenterCoord( 2, startpos, 0 )
			      : getCenterCoord( 2, startpos, 0 );
    }

    maniprecttrans->translation.setValue( x, y, z );
    maniprectscale->scaleFactor.setValue( xscale, yscale, 1 );

    maniprectswitch->whichChild = mIS_ZERO( z ) ? SO_SWITCH_NONE : 0;
}


void visBase::Rectangle::moveDraggertoManipRect()
{
    if ( !dragger ) return;

    float x = maniprecttrans->translation.getValue()[0];
    float y = maniprecttrans->translation.getValue()[1];
    float z = maniprecttrans->translation.getValue()[2];

    dragger->setCenter( x, y, z );

    float xscale = maniprectscale->scaleFactor.getValue()[0];
    float yscale = maniprectscale->scaleFactor.getValue()[1];

    dragger->setScale( xscale, yscale );
}


void visBase::Rectangle::setDraggerSize( float w, float h, float d )
{
    if ( !dragger ) return;

    w = getScale( 0, w );
    h = getScale( 0, h );

    dragger->setDraggerSize( w, h, d );
}



void visBase::Rectangle::moveObjectToManipRect(CallBacker*)
{
    SbVec3f centerpos( maniprecttrans->translation.getValue());
    SbVec3f scale = maniprectscale->scaleFactor.getValue();

    SbVec3f origopos( manipOrigo( 0 ), manipOrigo( 1 ), manipOrigo( 2 ) );

    if ( origotrans->translation.getValue()!=origopos )
    {
	origotrans->translation.setValue( origopos );
    }

    float newxwidth = getWidth( 0, scale[0] );
    float newywidth = getWidth( 1, scale[1] );

    if ( !mIS_ZERO( newxwidth-widthscale->scaleFactor.getValue()[0] ) ||
	 !mIS_ZERO( newywidth-widthscale->scaleFactor.getValue()[1] ) )
    {
	widthscale->scaleFactor.setValue(   newxwidth ,
					    newywidth, 1);
    }

    resetManip();
}


bool visBase::Rectangle::isManipRectOnObject() const
{
    bool res = true;
    SbVec3f centerpos( maniprecttrans->translation.getValue());
    SbVec3f scale = maniprectscale->scaleFactor.getValue();

    SbVec3f origopos( manipOrigo( 0 ), manipOrigo( 1 ), manipOrigo( 2 ) );

    if ( origotrans->translation.getValue()!=origopos )
    res = false;

    float newxwidth = getWidth( 0, scale[0] );
    float newywidth = getWidth( 1, scale[1] );

    if ( !mIS_ZERO( newxwidth-widthscale->scaleFactor.getValue()[0] ) ||
	 !mIS_ZERO( newywidth-widthscale->scaleFactor.getValue()[1] ) )
	res = false;

    return res;
}


void visBase::Rectangle::resetManip()
{
    maniprecttrans->translation.setValue( 0, 0, 0 );
    maniprectscale->scaleFactor.setValue( 1, 1, 1 );
    moveDraggertoManipRect();
}


float visBase::Rectangle::snapPos( int dim, float pos) const
{
    const StepInterval<float>& range =
			    !dim ? xrange : (dim==1 ? yrange : zrange );

    if ( snap && !mIsUndefined(range.step))
    {
	int idx = range.nearestIndex( pos );
	pos = range.start + idx*range.step;
    }

    pos = mMAX( pos, range.start );
    pos = mMIN( pos, range.stop );

    return pos;
}


float visBase::Rectangle::getWidth( int dim, float scale ) const
{
    return dim!=2 ? widthscale->scaleFactor.getValue()[dim] * scale: 0;
}


float visBase::Rectangle::getScale( int dim, float width ) const
{
    return dim!=2 ? width / widthscale->scaleFactor.getValue()[dim] : 0;
}


float visBase::Rectangle::getStartPos( int dim, float centerpos,
				       float scale ) const
{
    centerpos *= widthscale->scaleFactor.getValue()[dim] *
		 localscale->scaleFactor.getValue()[dim];

    centerpos += localorigotrans->translation.getValue()[dim] *
		 widthscale->scaleFactor.getValue()[dim];
    centerpos -= getWidth( dim, scale )/2;

    switch ( orientation_ )
    {
    case XY:
	centerpos +=origotrans->translation.getValue()[dim];
    break;
    case XZ:
	if ( dim==0 ) centerpos +=origotrans->translation.getValue()[0];
	else if ( dim==1 ) centerpos +=origotrans->translation.getValue()[2];
	else centerpos +=origotrans->translation.getValue()[1];
    break;
    case YZ:
	if ( dim==0 ) centerpos +=origotrans->translation.getValue()[2];
	else if ( dim==1 ) centerpos +=origotrans->translation.getValue()[1];
	else centerpos +=origotrans->translation.getValue()[0];
    break;
    }

    return centerpos;
}


float visBase::Rectangle::getStopPos( int dim, float centerpos,
				      float scale ) const 
{
    centerpos *= widthscale->scaleFactor.getValue()[dim] *
		 localscale->scaleFactor.getValue()[dim];

    centerpos += localorigotrans->translation.getValue()[dim] *
		 widthscale->scaleFactor.getValue()[dim];
    centerpos += getWidth( dim, scale )/2;

    switch ( orientation_ )
    {
    case XY:
	centerpos +=origotrans->translation.getValue()[dim];
    break;
    case XZ:
	if ( dim==0 ) centerpos +=origotrans->translation.getValue()[0];
	else if ( dim==1 ) centerpos +=origotrans->translation.getValue()[2];
	else centerpos +=origotrans->translation.getValue()[1];
    break;
    case YZ:
	if ( dim==0 ) centerpos +=origotrans->translation.getValue()[2];
	else if ( dim==1 ) centerpos +=origotrans->translation.getValue()[1];
	else centerpos +=origotrans->translation.getValue()[0];
    break;
    }

    return centerpos;
}


float visBase::Rectangle::getCenterCoord( int dim, float startpos,
					  float width ) const 
{
    switch ( orientation_ )
    {
    case XY:
	startpos -=origotrans->translation.getValue()[dim];
    break;
    case XZ:
	if ( dim==0 ) startpos -=origotrans->translation.getValue()[0];
	else if ( dim==1 ) startpos -=origotrans->translation.getValue()[2];
	else startpos -=origotrans->translation.getValue()[1];
    break;
    case YZ:
	if ( dim==0 ) startpos -=origotrans->translation.getValue()[2];
	else if ( dim==1 ) startpos -=origotrans->translation.getValue()[1];
	else startpos -=origotrans->translation.getValue()[0];
    break;
    }

    startpos += width /2;
    startpos -= localorigotrans->translation.getValue()[dim] *
		widthscale->scaleFactor.getValue()[dim];

    startpos /= widthscale->scaleFactor.getValue()[dim]*
		localscale->scaleFactor.getValue()[dim];

    return startpos;
}
