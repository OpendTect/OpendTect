/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visrectangle.cc,v 1.2 2002-02-12 13:37:57 kristofer Exp $";

#include "visrectangle.h"

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

visBase::Rectangle::Rectangle(bool userxymanip, bool userzmanip)
    : origotrans( new SoTranslation )
    , orientationrot( new SoRotation )
    , orientation( visBase::Rectangle::XY )
    , localorigotrans( new SoTranslation )
    , localscale( new SoScale )
    , widthscale( new SoScale )
    , planesep( new SoSeparator )
    , plane( new SoFaceSet )
    , manipswitch( 0 )
    , manipzdragger( 0 )
    , manipxydragger( 0 )
    , manipxyscale( 0 )
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
    float localwidth = 10;
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
    planesep->addChild( plane );
    plane->numVertices.set1Value(0, 5);
    registerSelection( plane );

    if ( userzmanip || userxymanip )
    {
	// Manip switch & separator
	manipswitch = new SoSwitch;
	root->addChild( manipswitch );

	SoSeparator* manipsep = new SoSeparator;
	manipswitch->addChild( manipsep );
	manipswitch->whichChild = SO_SWITCH_NONE;

	manipsep->addChild( new SoDrawStyle );

	// Manip rectangle
	SoSeparator* maniprectsep = new SoSeparator;
	manipsep->addChild( maniprectsep );

	maniprecttrans = new SoTranslation;
	maniprectsep->addChild( maniprecttrans );

	maniprectscale = new SoScale;
	maniprectsep->addChild( maniprectscale );

	SoMaterial* maniprectmaterial = new SoMaterial;
	maniprectsep->addChild( maniprectmaterial );
	maniprectmaterial->transparency.setValue( 0.5 );

	maniprectswitch = new SoSwitch;
	maniprectsep->addChild( maniprectswitch );
	maniprectswitch->addChild( plane );
	maniprectswitch->whichChild = SO_SWITCH_NONE;

	// Manips

	SoSeparator* manipzsep = new SoSeparator;
	manipsep->addChild( manipzsep );

	SoSeparator* manipxysep =  new SoSeparator;
	manipsep->addChild( manipxysep );


	if ( userzmanip )
	{
	    SoRotation* rot = new SoRotation;
	    manipzsep->addChild( rot );
	    rot->rotation.setValue( SbVec3f( 0,1,0), -M_PI/2 );

	    manipzdragger = new SoTranslate1Dragger;
	    manipzsep->addChild( manipzdragger );
	    addManipCB( manipzdragger );
	    manipzdragger->translation.setValue( 0, 1.1*hlocalwidth, 0 );
	}

	if ( userxymanip )
	{
	    manipxyscale = new SoScale;
	    manipxysep->addChild( manipxyscale );

	    manipxyscale->scaleFactor.setValue( hlocalwidth,
		    				hlocalwidth, hlocalwidth );

	    manipxydragger = new SoTabPlaneDragger;
	    manipxysep->addChild( manipxydragger );
	    addManipCB( manipxydragger );
	}
    }
}


visBase::Rectangle::~Rectangle()
{
    if ( manipxydragger ) removeManipCB( manipxydragger );
    if ( manipzdragger ) removeManipCB( manipzdragger );

    unregisterSelection( plane );
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

    switch ( orientation )
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
    widthscale->scaleFactor.setValue( x, y, (x+y)/2 );
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

    orientation = o;
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


void visBase::Rectangle::moveManipRectangletoDragger()
{
    if ( !manipswitch ) return;

    float x = maniprecttrans->translation.getValue()[0];
    float y = maniprecttrans->translation.getValue()[1];
    float z = maniprecttrans->translation.getValue()[2];

    float xscale = maniprectscale->scaleFactor.getValue()[0];
    float yscale = maniprectscale->scaleFactor.getValue()[1];

    float newx = x;
    float newy = y;
    float newz = z;

    float newxscale = xscale;
    float newyscale = yscale;

    if ( manipxydragger )
    {
	newx = manipxydragger->translation.getValue()[0] *
	       manipxyscale->scaleFactor.getValue()[0];
	newy = manipxydragger->translation.getValue()[1] *
	       manipxyscale->scaleFactor.getValue()[1];

	newxscale = manipxydragger->scaleFactor.getValue()[0];
	newyscale = manipxydragger->scaleFactor.getValue()[1];
    }

    if ( manipzdragger )
	newz = manipzdragger->translation.getValue()[0];

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
    

    startpos = snapPos(2, getStartPos( 2, orientation != XY ? -newz : newz, 0));
    if ( zrange.includes( startpos ) )
    {
	z = orientation != XY ? -getCenterCoord( 2, startpos, 0 )
			      : getCenterCoord( 2, startpos, 0 );
    }

    maniprecttrans->translation.setValue( x, y, z );
    maniprectscale->scaleFactor.setValue( xscale, yscale, 1 );

    maniprectswitch->whichChild = mIS_ZERO( z ) ? SO_SWITCH_NONE : 0;
}


void visBase::Rectangle::moveDraggertoManipRect()
{
    if ( !manipswitch ) return;

    float x = maniprecttrans->translation.getValue()[0];
    float y = maniprecttrans->translation.getValue()[1];
    float z = maniprecttrans->translation.getValue()[2];

    SbVec3f xyscale = manipxyscale ? manipxyscale->scaleFactor.getValue() 
				   : SbVec3f( 1, 1, 1 );
    float yscale = maniprectscale->scaleFactor.getValue()[1] * xyscale[1];

    bool oldstatus = enableManipCB( false );
    manipzdragger->translation.setValue( z, y + 1.1 * yscale, -x );

    manipxydragger->translation.setValue( x / xyscale[0], y / xyscale[1],
	    				  z / xyscale[2] );
    manipxydragger->scaleFactor.setValue(
	    maniprectscale->scaleFactor.getValue());
    enableManipCB( oldstatus );
}


bool visBase::Rectangle::moveObjectToManipRect()
{
    bool res = false;
    SbVec3f centerpos( maniprecttrans->translation.getValue());
    SbVec3f scale = maniprectscale->scaleFactor.getValue();

    SbVec3f origopos( manipOrigo( 0 ), manipOrigo( 1 ), manipOrigo( 2 ) );

    if ( origotrans->translation.getValue()!=origopos )
    {
	origotrans->translation.setValue( origopos );
	res = true;
    }

    float newxwidth = getWidth( 0, scale[0] );
    float newywidth = getWidth( 1, scale[1] );

    if ( !mIS_ZERO( newxwidth-widthscale->scaleFactor.getValue()[0] ) ||
	 !mIS_ZERO( newywidth-widthscale->scaleFactor.getValue()[1] ) )
    {
	widthscale->scaleFactor.setValue(   newxwidth ,
					    newywidth, (newxwidth+newywidth)/2);
	res = true;
    }

    resetManip();

    return res;
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



void visBase::Rectangle::updateDraggers()
{
    SbVec3f xyscale = manipxyscale ? manipxyscale->scaleFactor.getValue() 
				   : SbVec3f( 1, 1, 1 );

    float x = manipxydragger
		? manipxydragger->translation.getValue()[0] * xyscale[0] : 0;
    float y = manipxydragger 
		? manipxydragger->translation.getValue()[1] * xyscale[1] : 0;

    float z = manipzdragger ? manipzdragger->translation.getValue()[0] : 0;

    float yscale = manipxydragger
			? manipxydragger->scaleFactor.getValue()[1] * xyscale[1]
			: 2/localscale->scaleFactor.getValue()[1];

    bool oldstatus = enableManipCB( false );
    if ( manipzdragger )
    {
	manipzdragger->enableNotify( false );
	manipzdragger->translation.setValue( z, y + 1.1 * yscale, -x );
	manipzdragger->enableNotify( true );
    }

    if ( manipxydragger )
    {
	manipxydragger->enableNotify( false );
	manipxydragger->translation.setValue( x / xyscale[0],
					y / xyscale[1], z / xyscale[2] );
	manipxydragger->enableNotify( true );
    }

    enableManipCB( oldstatus );
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

    switch ( orientation )
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

    switch ( orientation )
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
    switch ( orientation )
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
