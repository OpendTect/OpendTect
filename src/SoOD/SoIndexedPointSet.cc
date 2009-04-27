/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

#include "SoIndexedPointSet.h"

#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/system/gl.h>

static const char* rcsID = "$Id: SoIndexedPointSet.cc,v 1.2 2009-04-27 21:22:19 cvskris Exp $";

SO_NODE_SOURCE(SoIndexedPointSet);


void SoIndexedPointSet::initClass()
{
    SO_NODE_INIT_CLASS(SoIndexedPointSet, SoIndexedShape, "IndexedShape");
}


SoIndexedPointSet::SoIndexedPointSet()
{
    SO_NODE_CONSTRUCTOR(SoIndexedPointSet);
}


#define mGetBindingImpl( Element ) \
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
	    binding = PER_VERTEX; \
	    break; \
	case Element::PER_FACE: \
	    binding = PER_VERTEX; \
	    break; \
	case Element::PER_PART_INDEXED: \
	    binding = PER_VERTEX_INDEXED; \
	    break; \
	case Element::PER_FACE_INDEXED: \
	    binding = PER_VERTEX_INDEXED; \
	    break; \
    } \
 \
    return binding

SoIndexedPointSet::Binding
SoIndexedPointSet::findNormalBinding(SoState* state) const
{
    mGetBindingImpl( SoNormalBindingElement );
}


SoIndexedPointSet::Binding
SoIndexedPointSet::findMaterialBinding(SoState* state) const
{
    mGetBindingImpl( SoMaterialBindingElement );
}


void SoIndexedPointSet::GLRender(SoGLRenderAction* action)
{
    if ( !coordIndex.getNum() || !shouldGLRender(action) )
	return;

    SoState* state = action->getState();

    SbBool didpush = false;
    if ( vertexProperty.getValue() )
    {
	state->push();
	didpush = true;
	vertexProperty.getValue()->GLRender( action );
    }

    SoTextureCoordinateBundle tb( action, TRUE, FALSE );
    bool dotextures = tb.needCoordinates();
    SoMaterialBundle mb(action);

    bool neednormals = !mb.isColorOnly() || tb.isFunction();


    const SoCoordinateElement* coords;
    const SbVec3f* normals;
    int numindices;
    const int32_t* cindices, *nindices, *tindices, *mindices;
    SbBool normalcacheused;

    getVertexData(state, coords, normals, cindices,
		    nindices, tindices, mindices, numindices,
		    neednormals, normalcacheused);

    if ( !normals && neednormals )
    {
	neednormals = false;
	if ( !didpush )
	{
	    state->push();
	    didpush = true;
	}

	SoLazyElement::setLightModel( state, SoLazyElement::BASE_COLOR );
    }

    Binding mbind = findMaterialBinding(state);
    Binding nbind = OVERALL;
    if ( neednormals ) nbind = findNormalBinding(state);

    if ( nbind==OVERALL && neednormals )
    {
	if ( normals )
	    glNormal3fv(normals[0].getValue());
	else
	    glNormal3f(0.0f, 0.0f, 1.0f);
    }

    mb.sendFirst();

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

    const int32_t* viptr = cindices;
    const int32_t* viendptr = viptr + numindices;
    SbVec3f point;

    int texidx = 0;
    int matnr = 0;
    int normnr = 0;

    glBegin( GL_POINTS );

    if ( mbind==OVERALL )
	mb.send( 0, true );

    while ( viptr<viendptr )
    {
    	int32_t v1 = *viptr++;
	if ( mbind==PER_VERTEX )
	    mb.send( matnr++, true );
	else if ( mbind==PER_VERTEX_INDEXED )
	{
	    const int mindex = mindices ? mindices[matnr++] : 0;
	    mb.send( mindex, true );
	}

	if ( nbind==PER_VERTEX )
	    glNormal3fv( normals[normnr++].getValue() );
	else if ( nbind==PER_VERTEX_INDEXED )
	    glNormal3fv(normals[*nindices++].getValue());

	if ( dotextures )
	{
	    if ( tb.isFunction() )
	    {
		SbVec3f texturecoord;
		tb.get(coords->get3(v1), texturecoord);
		glTexCoord2fv( texturecoord.getValue() );
	    }
	    else
	    {
		int32_t t1 = tindices ? *tindices++ : texidx++;
		glTexCoord2fv( tb.get(t1).getValue() );
	    }
	}

	glVertex3fv( coords->get3(v1).getValue() );
    }

    glEnd();

    if ( didpush )
	state->pop();
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

void SoIndexedPointSet::generatePrimitives(SoAction* action)
{
    if ( !coordIndex.getNum() )
	return;

    SoState* state = action->getState();

    if ( vertexProperty.getValue() )
    {
	state->push();
	vertexProperty.getValue()->doAction( action );
    }

    const SoCoordinateElement* coords;
    const SbVec3f* normals;
    const int32_t* cindices;
    int numindices;
    const int32_t* nindices;
    const int32_t* tindices;
    const int32_t* mindices;
    SbBool neednormals = true;
    SbBool normalcacheused;

    getVertexData(state, coords, normals, cindices,
		  nindices, tindices, mindices, numindices,
		  neednormals, normalcacheused);

    if ( !normals ) neednormals = false;

    SoTextureCoordinateBundle tb(action, FALSE, FALSE);
    bool dotextures = tb.needCoordinates();

    Binding mbind = this->findMaterialBinding(state);
    Binding nbind = this->findNormalBinding(state);

    if ( !neednormals ) nbind = OVERALL;

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
    if ( nbind==OVERALL )
    {
	if ( normals )
	    currnormal = normals;
    }

    int texidx = 0;
    int matnr = 0;
    int normnr = 0;

    const int32_t* viptr = cindices;
    const int32_t* viendptr = viptr + numindices;

    SoPrimitiveVertex vertex;
    SoPointDetail pointdetail;

    vertex.setNormal(*currnormal);
    vertex.setDetail(&pointdetail);

    beginShape(action, SoShape::POINTS );
    while ( viptr<viendptr )
    {
    	int32_t v1 = *viptr++;
	if ( nbind==PER_VERTEX )
	{
	    pointdetail.setNormalIndex( normnr );
	    currnormal = &normals[normnr++];
	    vertex.setNormal( *currnormal );
	}
	else if ( nbind==PER_VERTEX_INDEXED )
	{
	    pointdetail.setNormalIndex( nindices[normnr] );
	    currnormal = &normals[nindices[normnr++]];
	    vertex.setNormal( *currnormal );
	}

	if ( mbind==PER_VERTEX )
	{
	    pointdetail.setMaterialIndex( matnr );
	    vertex.setMaterialIndex( matnr++ );
	}
	else if ( mbind==PER_VERTEX_INDEXED )
	{
	    pointdetail.setMaterialIndex( mindices[matnr] );
	    vertex.setMaterialIndex( mindices[matnr++] );
	}

	if ( dotextures )
	{
	    if ( tb.isFunction() )
		vertex.setTextureCoords(tb.get(coords->get3(v1), *currnormal));
	    else 
	    {
		int32_t t1 = tindices ? *tindices++ : texidx++;
		glTexCoord2fv( tb.get(t1).getValue() );
		pointdetail.setTextureCoordIndex( t1 );
		vertex.setTextureCoords( tb.get(t1) );
	    }
	}

	pointdetail.setCoordinateIndex( v1 );
	vertex.setPoint(coords->get3( v1 ));
	shapeVertex( &vertex );
    }

    endShape();

    if ( vertexProperty.getValue() )
	state->pop();

    if ( normalcacheused )
	readUnlockNormalCache();
}


SbBool SoIndexedPointSet::generateDefaultNormals( SoState* state,
						  SoNormalCache* nc)
{
    nc->set( 0, 0 );
    return true;
}
