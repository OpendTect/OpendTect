/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

#include "SoIndexedTriangleFanSet.h"

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoCreaseAngleElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoTextureCoordinateBindingElement.h>
#include <Inventor/system/gl.h>

static const char* rcsID = "$Id: SoIndexedTriangleFanSet.cc,v 1.9 2009/07/22 16:01:35 cvsbert Exp $";

SO_NODE_SOURCE(SoIndexedTriangleFanSet);


void SoIndexedTriangleFanSet::initClass()
{
    SO_NODE_INIT_CLASS(SoIndexedTriangleFanSet, SoIndexedShape, "IndexedShape");
}


SoIndexedTriangleFanSet::SoIndexedTriangleFanSet()
{
    SO_NODE_CONSTRUCTOR(SoIndexedTriangleFanSet);
}


#define mgetBindingImpl( Element ) \
    Binding binding = PER_VERTEX_INDEXED; \
    Element::Binding normbind = Element::get(state); \
 \
    switch (normbind) \
    { \
	case Element::OVERALL: \
	    binding = OVERALL; \
	    break; \
	case Element::PER_VERTEX: \
	    binding = PER_VERTEX; \
	    break; \
	case Element::PER_VERTEX_INDEXED: \
	    binding = PER_VERTEX_INDEXED; \
	    break; \
	case Element::PER_PART: \
	    binding = PER_FAN; \
	    break; \
	case Element::PER_FACE: \
	    binding = PER_TRIANGLE; \
	    break; \
	case Element::PER_PART_INDEXED: \
	    binding = PER_FAN_INDEXED; \
	    break; \
	case Element::PER_FACE_INDEXED: \
	    binding = PER_TRIANGLE_INDEXED; \
	    break; \
    } \
 \
    return binding

SoIndexedTriangleFanSet::Binding
SoIndexedTriangleFanSet::findNormalBinding(SoState* state) const
{
    mgetBindingImpl( SoNormalBindingElement );
}


SoIndexedTriangleFanSet::Binding
SoIndexedTriangleFanSet::findMaterialBinding(SoState* state) const
{
    mgetBindingImpl( SoMaterialBindingElement );
}


#define mGLSendVertex(vertexindex, materialcond, indexedmaterialcond, \
			normalcond, indexednormalcond) \
	if ( materialcond ) \
	{ \
	} \
	else if ( indexedmaterialcond ) \
	{ \
	} \
 \
	if ( normalcond ) \
	{ \
	    glNormal3fv(normals[normnr++].getValue());\
	} \
	else if ( indexednormalcond ) \
	{ \
	    glNormal3fv(normals[*nindices++].getValue()); \
	} \
 \
	if ( dotextures ) \
	{ \
	    if ( tb.isFunction() ) \
	    { \
		SbVec3f texturecoord;\
		tb.get(coords->get3(vertexindex), texturecoord);\
		glTexCoord2fv( texturecoord.getValue() ); \
	    } \
	    else \
	    { \
		glTexCoord2fv(tb.get(tindices?*tindices++:texidx++).getValue()); \
	    } \
	} \
	glVertex3fv(coords->get3(vertexindex).getValue())

void SoIndexedTriangleFanSet::GLRender(SoGLRenderAction* action)
{
    if ( coordIndex.getNum()<3 ) return;

    SbBool didpush = false;
    SoState* state = action->getState();

    Binding mbind = findMaterialBinding(state);
    Binding nbind = findNormalBinding(state);

    if ( nbind==PER_TRIANGLE || nbind==PER_TRIANGLE_INDEXED ||
	 mbind==PER_TRIANGLE || mbind==PER_TRIANGLE_INDEXED )
    {
	if ( !didpush )
	{
	    state->push();
	    didpush = true;
	}

	SoLazyElement::setShadeModel(state, true);
    }

    if ( !shouldGLRender(action) )
    {
	if ( didpush )
	    state->pop();
	return;
    }

    const SoCoordinateElement* coords;
    const SbVec3f* normals;
    const int32_t* cindices;
    int numindices;
    const int32_t* nindices;
    const int32_t* tindices;
    const int32_t* mindices;
    SbBool dotextures;
    SbBool normalcacheused;
    SoMaterialBundle mb(action);

    SbBool sendnormals = !mb.isColorOnly();

    getVertexData(state, coords, normals, cindices,
		    nindices, tindices, mindices, numindices,
		    sendnormals, normalcacheused);

    SoTextureCoordinateBundle tb(action, TRUE, FALSE);
    dotextures = tb.needCoordinates();

    if ( dotextures )
    {
	if ( tb.isFunction() )
	    tindices = 0;
        else if ( SoTextureCoordinateBindingElement::get(state) ==
			SoTextureCoordinateBindingElement::PER_VERTEX )
	    tindices = 0;
	else if ( !tindices )
	    tindices = cindices;
    }

    if ( !sendnormals )
	nbind = OVERALL;

    else if ( nbind==OVERALL)
    {
	if ( normals )
	    glNormal3fv(normals[0].getValue());
	else
	    glNormal3f(0.0f, 0.0f, 1.0f);
    }
    else if ( normalcacheused && nbind==PER_VERTEX )
	nbind = PER_VERTEX_INDEXED;
    else if ( normalcacheused && nbind==PER_TRIANGLE_INDEXED)
	nbind = PER_TRIANGLE;
    else if ( normalcacheused && nbind==PER_FAN_INDEXED )
	nbind = PER_FAN;

    mb.sendFirst(); // make sure we have the correct material

    const int32_t* viptr = cindices;
    const int32_t* viendptr = viptr + numindices;
    SbVec3f point;

    int texidx = 0;
    int matnr = 0;
    int normnr = 0;

    while ( viptr+2<viendptr )
    {
    	int32_t v1 = *viptr++;
    	const int32_t v2 = *viptr++;
    	const int32_t v3 = *viptr++;
	glBegin(GL_TRIANGLE_FAN);

	mGLSendVertex( v1,
		mbind==PER_VERTEX || mbind==PER_TRIANGLE || mbind==PER_FAN,
		mbind==PER_VERTEX_INDEXED || mbind==PER_TRIANGLE_INDEXED ||
		    mbind==PER_FAN_INDEXED,
		nbind==PER_VERTEX || nbind==PER_TRIANGLE || nbind==PER_FAN,
		nbind==PER_VERTEX_INDEXED || nbind==PER_TRIANGLE_INDEXED ||
		    nbind==PER_FAN_INDEXED );

	mGLSendVertex( v2,
		mbind==PER_VERTEX,
		mbind==PER_VERTEX_INDEXED,
		nbind==PER_VERTEX,
		nbind==PER_VERTEX_INDEXED );

	mGLSendVertex( v3,
		mbind==PER_VERTEX,
		mbind==PER_VERTEX_INDEXED,
		nbind==PER_VERTEX,
		nbind==PER_VERTEX_INDEXED );

	v1 = viptr<viendptr ? *viptr++ : -1;
	while ( v1>=0 )
	{
	    mGLSendVertex( v1,
		    mbind==PER_VERTEX || mbind==PER_TRIANGLE,
		    mbind==PER_VERTEX_INDEXED || mbind==PER_TRIANGLE_INDEXED,
		    nbind==PER_VERTEX || nbind==PER_TRIANGLE,
		    nbind==PER_VERTEX_INDEXED || nbind==PER_TRIANGLE_INDEXED);

	    v1 = viptr < viendptr ? *viptr++ : -1;
	}

	if ( nindices ) nindices++;
	if ( tindices ) tindices++;
	if ( mindices ) mindices++;

        glEnd();
    }

    if ( didpush )
	state->pop();

    return;
}


#define mSendVertex(vertexindex, materialcond, indexedmaterialcond, \
			normalcond, indexednormalcond) \
	if ( materialcond ) \
	{ \
	    pointdetail.setMaterialIndex(matnr); \
	    vertex.setMaterialIndex(matnr++); \
	} \
	else if ( indexedmaterialcond ) \
	{ \
	    pointdetail.setMaterialIndex(*mindices); \
	    vertex.setMaterialIndex(*mindices++); \
	} \
 \
	if ( normalcond ) \
	{ \
	    pointdetail.setNormalIndex(normnr); \
	    currnormal = &normals[normnr++]; \
	    vertex.setNormal(*currnormal); \
	} \
	else if ( indexednormalcond ) \
	{ \
	    pointdetail.setNormalIndex(*nindices); \
	    currnormal = &normals[*nindices++]; \
	    vertex.setNormal(*currnormal); \
	} \
 \
	if ( dotextures ) \
	{ \
	    if ( tb.isFunction() ) \
		vertex.setTextureCoords(tb.get(coords->get3(vertexindex), \
			    			*currnormal)); \
	    else \
	    { \
		pointdetail.setTextureCoordIndex(tindices?*tindices:texidx); \
		vertex.setTextureCoords(tb.get(tindices?*tindices++:texidx++)); \
	    } \
	} \
	pointdetail.setCoordinateIndex(vertexindex); \
	vertex.setPoint(coords->get3(vertexindex)); \
	shapeVertex(&vertex)

void SoIndexedTriangleFanSet::generatePrimitives(SoAction* action)
{
    if ( coordIndex.getNum()<3 )
	return;

    SoState* state = action->getState();

    Binding mbind = this->findMaterialBinding(state);
    Binding nbind = this->findNormalBinding(state);

    const SoCoordinateElement* coords;
    const SbVec3f* normals;
    const int32_t* cindices;
    int numindices;
    const int32_t* nindices;
    const int32_t* tindices;
    const int32_t* mindices;
    SbBool dotextures;
    SbBool sendnormals = TRUE;
    SbBool normalcacheused;

    getVertexData(state, coords, normals, cindices,
		  nindices, tindices, mindices, numindices,
		  sendnormals, normalcacheused);

    SoTextureCoordinateBundle tb(action, FALSE, FALSE);
    dotextures = tb.needCoordinates();

    if ( dotextures )
    {
	if ( tb.isFunction() )
	    tindices = 0;
	else if ( SoTextureCoordinateBindingElement::get(state) ==
	          SoTextureCoordinateBindingElement::PER_VERTEX )
	    tindices = 0;
	else if ( !tindices )
	    tindices = cindices;
    }

    if ( nbind==PER_VERTEX_INDEXED && !nindices )
	nindices = cindices;

    if (mbind == PER_VERTEX_INDEXED && !mindices )
	mindices = cindices;

    SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
    const SbVec3f* currnormal = &dummynormal;
    if ( !sendnormals )
	nbind = OVERALL;
    else if ( nbind==OVERALL )
    {
	if ( normals )
	    currnormal = normals;
    }
    else if ( normalcacheused && nbind==PER_VERTEX )
	nbind = PER_VERTEX_INDEXED;
    else if ( normalcacheused && nbind==PER_TRIANGLE_INDEXED )
	nbind = PER_TRIANGLE;
    else if ( normalcacheused && nbind==PER_FAN_INDEXED)
	nbind = PER_FAN;

    int texidx = 0;
    int matnr = 0;
    int normnr = 0;

    const int32_t* viptr = cindices;
    const int32_t* viendptr = viptr + numindices;

    SoPrimitiveVertex vertex;
    SoPointDetail pointdetail;
    SoFaceDetail facedetail;

    vertex.setNormal(*currnormal);
    vertex.setDetail(&pointdetail);


    while ( viptr+1<viendptr )
    {
	facedetail.setFaceIndex(0);

    	int32_t v1 = *viptr++;
    	const int32_t v2 = *viptr++;
    	const int32_t v3 = *viptr++;
	beginShape(action, TRIANGLE_FAN, &facedetail);
	mSendVertex( v1,
		mbind==PER_VERTEX || mbind==PER_TRIANGLE || mbind==PER_FAN,
		mbind==PER_VERTEX_INDEXED || mbind==PER_TRIANGLE_INDEXED ||
		    mbind==PER_FAN_INDEXED,
		nbind==PER_VERTEX || nbind==PER_TRIANGLE || nbind==PER_FAN,
		nbind==PER_VERTEX_INDEXED || nbind==PER_TRIANGLE_INDEXED ||
		    nbind==PER_FAN_INDEXED );

	mSendVertex( v2,
		mbind==PER_VERTEX,
		mbind==PER_VERTEX_INDEXED,
		nbind==PER_VERTEX,
		nbind==PER_VERTEX_INDEXED );

	mSendVertex( v3,
		mbind==PER_VERTEX,
		mbind==PER_VERTEX_INDEXED,
		nbind==PER_VERTEX,
		nbind==PER_VERTEX_INDEXED );

	facedetail.incFaceIndex();

	v1 = viptr<viendptr ? *viptr++ : -1;
	while ( v1>=0 )
	{
	    mSendVertex( v1,
		    mbind==PER_VERTEX || mbind==PER_TRIANGLE,
		    mbind==PER_VERTEX_INDEXED || mbind==PER_TRIANGLE_INDEXED,
		    nbind==PER_VERTEX || nbind==PER_TRIANGLE,
		    nbind==PER_VERTEX_INDEXED || nbind==PER_TRIANGLE_INDEXED);

	    facedetail.incFaceIndex();
	    v1 = viptr < viendptr ? *viptr++ : -1;
	}

	endShape();
	facedetail.incFaceIndex();

	if ( mbind==PER_VERTEX_INDEXED )
	    mindices++;
	if ( nbind==PER_VERTEX_INDEXED )
	    nindices++;
	if ( tindices )
	    tindices++;
    }

    if ( normalcacheused )
	readUnlockNormalCache();
}


SbBool SoIndexedTriangleFanSet::generateDefaultNormals( SoState* state,
							SoNormalCache* nc)
{
    /* This code is not correct, but we do normally not use
        the fanset with auto-generated normals. Notify kristofer
        if you need to do that
    */
    if ( coordIndex.getNum()<3 )
	return false;

    const SoCoordinateElement * coordelem =
	       SoCoordinateElement::getInstance(state);

    SbBool ccw = SoShapeHintsElement::getVertexOrdering(state)!=
	    		SoShapeHintsElement::CLOCKWISE;

    SoVertexProperty* vp = (SoVertexProperty*) vertexProperty.getValue();
    SbBool vpvtx = vp && (vp->vertex.getNum()>0);
    SbBool vpnorm = vp && (vp->normal.getNum()>0);

    const SbVec3f* coords = vpvtx
			?  vp->vertex.getValues(0)
			: coordelem->getArrayPtr3();

    SoNormalBindingElement::Binding normbind = vpnorm
	? (SoNormalBindingElement::Binding) vp->normalBinding.getValue()
	: SoNormalBindingElement::get(state);


    switch (normbind)
    {
	case SoNormalBindingElement::PER_VERTEX:
	case SoNormalBindingElement::PER_VERTEX_INDEXED:
#if COIN_MAJOR_VERSION >= 3
	    nc->generatePerVertex(coords,
				    vp->vertex.getNum(),
				    coordIndex.getValues(0),
				    coordIndex.getNum(),
				    SoCreaseAngleElement::get(state),
				    0, ccw, true);
#else
	    nc->generatePerVertex(coords,
				    coordIndex.getValues(0),
				    coordIndex.getNum(),
				    SoCreaseAngleElement::get(state),
				    0, ccw, true);
#endif

	    break;
	case SoNormalBindingElement::PER_FACE:
	case SoNormalBindingElement::PER_FACE_INDEXED:
#if COIN_MAJOR_VERSION >= 3
	    nc->generatePerFaceStrip(coords,
				    vp->vertex.getNum(),
				    coordIndex.getValues(0),
				    coordIndex.getNum(),
				    ccw);
#else
	    nc->generatePerFaceStrip(coords,
				    coordIndex.getValues(0),
				    coordIndex.getNum(),
				    ccw);
#endif
	break;

	case SoNormalBindingElement::PER_PART:
	case SoNormalBindingElement::PER_PART_INDEXED:
#if COIN_MAJOR_VERSION >= 3
	    nc->generatePerStrip(coords,
				    vp->vertex.getNum(),
				    coordIndex.getValues(0),
				    coordIndex.getNum(),
				    ccw);
#else
	    nc->generatePerStrip(coords,
				    coordIndex.getValues(0),
				    coordIndex.getNum(),
				    ccw);
#endif
	    break;
	default:
	    break;
    }

    return true;
}
