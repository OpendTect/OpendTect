/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoPolygonSelect.cc,v 1.5 2009-10-14 08:09:49 cvsjaap Exp $";


#include "SoPolygonSelect.h"

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/caches/SoCache.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/system/gl.h>


SO_NODE_SOURCE(SoPolygonSelect);

void SoPolygonSelect::initClass(void)
{
    SO_NODE_INIT_CLASS(SoPolygonSelect, SoNode, "Node");
    SO_ENABLE( SoGLRenderAction, SoViewVolumeElement );
    SO_ENABLE( SoHandleEventAction, SoViewportRegionElement );
}


SoPolygonSelect::SoPolygonSelect(void) 
    : mousedown_( false )
    , isgrabbing_( false )
    , dependencychecker_( 0 )
{
    SO_NODE_CONSTRUCTOR( SoPolygonSelect );

    SO_NODE_ADD_FIELD( mode, (SoPolygonSelect::POLYGON) );

    SO_NODE_DEFINE_ENUM_VALUE( Modes, OFF );
    SO_NODE_DEFINE_ENUM_VALUE( Modes, RECTANGLE );
    SO_NODE_DEFINE_ENUM_VALUE( Modes, POLYGON );
    SO_NODE_SET_SF_ENUM_TYPE( mode, Modes );
}


SoPolygonSelect::~SoPolygonSelect()
{
    if ( dependencychecker_ ) dependencychecker_->unref();
}


SbVec2f SoPolygonSelect::projectPoint( const SbVec3f& pt ) const
{
    SbVec3f modelpt;
    modelmatrix_.multMatrixVec( pt, modelpt );

    SbVec3f res;
    vv_.projectToScreen( modelpt, res );

    if ( res[2] > 1.0 )
	return SbVec2f( 1e30, 1e30 ); // Undefined

    return SbVec2f( res[0], res[1] );
}


void SoPolygonSelect::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();

    if ( dependencychecker_ && !dependencychecker_->isValid(state) )
    {
	polygon_.truncate( 0 );
	mousedown_ = false;
	dependencychecker_->unref();
	dependencychecker_ = 0;
	polygonChange.invokeCallbacks( this );
    }

    if ( !dependencychecker_ )
    {
	const SbBool storedinvalid = SoCacheElement::setInvalid( false ); 
	const SoElement* elt =
	    state->getConstElement( SoViewVolumeElement::getClassStackIndex());
	state->push();

	dependencychecker_ = new SoCache( state );
	dependencychecker_->ref();
	dependencychecker_->addElement( elt );
	SoCacheElement::set(state, dependencychecker_); 
	state->pop();
	SoCacheElement::setInvalid(storedinvalid); 
	modelmatrix_ = SoModelMatrixElement::get(state);
	vv_ = SoViewVolumeElement::get(state);
    }

    if ( polygon_.getLength()<2 )
	return;

    SoMaterialBundle mb(action);
    mb.sendFirst();

    const SbMatrix world2model = modelmatrix_.inverse();

    glBegin( GL_LINE_LOOP );
    const SbPlane plane = vv_.getPlane( 0 );
    const SbVec3f normal = plane.getNormal();
    glNormal3fv( normal.getValue() );

    const float zpos = vv_.nearDist + 0.001*vv_.nearToFar;

    for ( int idx=0; idx<polygon_.getLength(); idx++ )
    {
	const SbVec3f worldvec = vv_.getPlanePoint( zpos, polygon_[idx] );
	SbVec3f modelvec;
	world2model.multMatrixVec( worldvec, modelvec );
	glVertex3fv( modelvec.getValue() );
    }

    glEnd();
}


void SoPolygonSelect::handleEvent( SoHandleEventAction* action )
{
    if ( action->isHandled() )
	return;

    if ( mode.getValue()==OFF )
	return;

    SoState* state = action->getState();
    const SoEvent* event = action->getEvent();

    if ( mousedown_ && SoMouseButtonEvent::isButtonReleaseEvent (
		                    event, SoMouseButtonEvent::BUTTON1 ))
    {
	if ( isgrabbing_ )
	    action->releaseGrabber();

	isgrabbing_ = false;

	mousedown_ = false;
	action->setHandled();
	paintStop.invokeCallbacks( this );
    }
    else if ( mousedown_ &&
	      event->getTypeId()==SoLocation2Event::getClassTypeId() )
    {
	action->setHandled();
	const SbVec2f pt =
	    event->getNormalizedPosition(SoViewportRegionElement::get(state));
	if ( mode.getValue()==RECTANGLE )
	{
	    if ( polygon_.getLength()!=4 )
	    {
		polygon_.append( SbVec2f(0,0) );
		polygon_.append( SbVec2f(0,0) );
		polygon_.append( SbVec2f(0,0) );
	    }

	    const SbVec2f& origin = polygon_[0];
	    polygon_[1] = SbVec2f( origin[0], pt[1] );
	    polygon_[2] = pt;
	    polygon_[3] = SbVec2f( pt[0], origin[1] );
	}
	else
	{
	    if ( polygon_.getLength()>10000 )
		return;

	    polygon_.append( pt );
	}

	polygonChange.invokeCallbacks( this );
	touch();
    }
    else if ( SoMouseButtonEvent::isButtonPressEvent (
		    event, SoMouseButtonEvent::BUTTON1 ) )
    {
	mousedown_ = true;
	action->setHandled();
	if ( !action->getGrabber() )
	{
	    isgrabbing_ = true;
	    action->setGrabber( this );
	}
	else
	    isgrabbing_ = false;

	polygon_.truncate( 0 );
	const SbVec2f pt =
	    event->getNormalizedPosition(SoViewportRegionElement::get(state));
	polygon_.append( pt );
	paintStart.invokeCallbacks( this );
	polygonChange.invokeCallbacks( this );
	touch();
    }
}
