/*+
________________________________________________________________________

  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  Author:        Kristofer
  Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoLineSet3D.cc,v 1.3 2011/09/26 15:28:54 cvsyuancheng Exp $";

#include "SoLineSet3D.h"

#include <Inventor/SbLinear.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoPointDetail.h>

#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoTextureCoordinateBindingElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/system/gl.h>

#include <Inventor/errors/SoDebugError.h>

#include "SoCameraInfo.h"
#include "SoCameraInfoElement.h"

SO_NODE_SOURCE(SoLineSet3D);

void SoLineSet3D::initClass()
{
    SO_NODE_INIT_CLASS(SoLineSet3D, SoNonIndexedShape, "NonIndexedShape");
    SO_ENABLE( SoGLRenderAction, SoCameraInfoElement );
    SO_ENABLE( SoGLRenderAction, SoModelMatrixElement );
    SO_ENABLE( SoGLRenderAction, SoViewportRegionElement );
    SO_ENABLE( SoGLRenderAction, SoViewVolumeElement );
    SO_ENABLE( SoGLRenderAction, SoCacheElement );
}


SoLineSet3D::SoLineSet3D()
{
    SO_NODE_CONSTRUCTOR(SoLineSet3D);
    SO_NODE_ADD_FIELD( radius, (5.0) );
    SO_NODE_ADD_FIELD( screenSize, (true) );
    SO_NODE_ADD_FIELD( maxRadius, (-1) );
    SO_NODE_ADD_FIELD( numVertices, (-1) );
}


SoLineSet3D::~SoLineSet3D()
{ }


#define mRenderQuad( c1, c2, m12, c3, c4, m34, norm23, norm14 )\
    mb.send(m12,true);\
    if ( isreversed ) \
    { \
	glNormal3fv((norm23).getValue()); \
	glVertex3fv((c2).getValue());\
	glNormal3fv((norm14).getValue()); \
	glVertex3fv((c1).getValue());\
	mb.send(m34,true);\
	glNormal3fv((norm14).getValue()); \
	glVertex3fv((c4).getValue());\
	glNormal3fv((norm23).getValue()); \
	glVertex3fv((c3).getValue());\
    } \
    else \
    { \
	glNormal3fv((norm14).getValue()); \
	glVertex3fv((c1).getValue());\
	glNormal3fv((norm23).getValue()); \
	glVertex3fv((c2).getValue());\
	mb.send(m34,true);\
	glNormal3fv((norm23).getValue()); \
	glVertex3fv((c3).getValue());\
	glNormal3fv((norm14).getValue()); \
	glVertex3fv((c4).getValue());\
    }


void SoLineSet3D::GLRender(SoGLRenderAction* action)
{
    if ( !shouldGLRender(action) )
	return;

    SoState* state = action->getState();

#ifdef USE_DISPLAYLIST_LINESET
    if ( !displaylist_ )
	displaylist_ = new SoGLDisplayList( state, 
		SoGLDisplayList::DISPLAY_LIST );
    displaylist_->ref();
    if ( !displaylist_ )
    {
	SoDebugError::postWarning( "SoLineSet3D::GLRender",
	       "Cannot create display list!" );
	return;
    }

    if ( SoCacheElement::anyOpen( state ) )
    {
	SoDebugError::postWarning( "SoLineSet3D::GLRender",
 	"A cache is already open! Cannot generate coordinates now!" );
	return;
    }
#endif

    bool isvalid = data_.areCoordsValid( state, this, screenSize.getValue() );

    if ( !isvalid )
    {
	const SoCoordinateElement* celem =
	    SoCoordinateElement::getInstance(state); 
	const int nrcoords = celem->getNum();

	SbList<int32_t> ci( nrcoords*2 );
	int nci = 0;
	int curc = 0;
	int nrlines = numVertices.getNum();
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    const int nrvertex = numVertices[idx];
	    int lastc;
	    if ( nrvertex==-1 )
		lastc = nrcoords-1;
	    else
		lastc = curc+nrvertex-1;

	    while ( curc<=lastc )
		ci[nci++] = curc++;

	    ci[nci++] = -1;
	}

	data_.generateCoordinates( this, radius.getValue(),
		screenSize.getValue(), maxRadius.getValue(),
		ci.getArrayPtr(0), nci, state );
    }

#ifdef USE_DISPLAYLIST_LINESET
    if ( isvalid )
    {
	displaylist_->call( state );
	return;
    }
    else
        displaylist_->open( state );
#endif

    data_.glRender( 0, action );
}


void SoLineSet3D::generatePrimitives(SoAction* action)
{
}


void SoLineSet3D::computeBBox( SoAction* action, SbBox3f& box, SbVec3f& center )
{
    int32_t numvertices = 0;
    for ( int i=0; i<this->numVertices.getNum(); i++ )
	numvertices += this->numVertices[i];
    SoNonIndexedShape::computeCoordBBox(action, numvertices, box, center);
}


void SoLineSet3D::rayPick( SoRayPickAction* action )
{
    SoState* state = action->getState();
    SbBox3f box;
    SbVec3f dummy;

    computeBBox( action, box, dummy );
    if ( !action->intersect(box,dummy) )
	return;

    const SoCoordinateElement* celem = SoCoordinateElement::getInstance(state);
    const int nrcoords = celem->getNum();

    int curc = 0;
    int nrlines = numVertices.getNum();
    for ( int idx=0; idx<nrlines; idx++ )
    {
	const int nrvertex = numVertices[idx];
	int lastc;
	if ( nrvertex==-1 )
	    lastc = nrcoords-1;
	else
	    lastc = curc+nrvertex-1;

	SbVec3f c1 = celem->get3(curc++);
	while ( curc<lastc )
	{
	    SbVec3f c2 = celem->get3(curc++);
	    if ( action->intersect( c1, c2, dummy ) )
	    {
		SoPickedPoint* pickedpoint = action->addIntersection(dummy);
		//Todo: Fill out pickedpoint
	    }
	    c1 = c2;
	}
    }
}
