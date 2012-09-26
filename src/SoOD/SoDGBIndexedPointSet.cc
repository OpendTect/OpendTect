/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

#include "SoDGBIndexedPointSet.h"

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

static const char* rcsID mUsedVar = "$Id$";

SO_NODE_SOURCE(SoDGBIndexedPointSet);


void SoDGBIndexedPointSet::initClass()
{
    SO_NODE_INIT_CLASS(SoDGBIndexedPointSet, SoIndexedShape, "IndexedShape");
}


SoDGBIndexedPointSet::SoDGBIndexedPointSet()
{
    SO_NODE_CONSTRUCTOR(SoDGBIndexedPointSet);
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

SoDGBIndexedPointSet::Binding
SoDGBIndexedPointSet::findNormalBinding(SoState* state) const
{
    mGetBindingImpl( SoNormalBindingElement );
}


SoDGBIndexedPointSet::Binding
SoDGBIndexedPointSet::findMaterialBinding(SoState* state) const
{
    mGetBindingImpl( SoMaterialBindingElement );
}


void SoDGBIndexedPointSet::GLRender(SoGLRenderAction* action)
{
    if ( !shouldGLRender(action) )
	return;

    //if ( !coordIndex.getNum() || (coordIndex.getNum()==1 && coordIndex[0]==-1) )
	//return;

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
    const int32_t* cindices;
    const int32_t* nindices;
    const int32_t* tindices;
    const int32_t* mindices;
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
    int matidx = 0;
    int normidx = 0;

    glBegin( GL_POINTS );

    if ( mbind==OVERALL )
	mb.send( 0, true );

    while ( viptr<viendptr )
    {
    	int32_t v1 = *viptr++;
	int32_t t1 = tindices ? *tindices++ : texidx++;
	int32_t n1 = nindices ? *nindices++ : normidx++;
	int32_t m1 = mindices ? *mindices++ : matidx++;

	if ( v1<0 )
	    continue;

	if ( mbind==PER_VERTEX || mbind==PER_VERTEX_INDEXED )
	    mb.send( m1, true );

	if ( nbind==PER_VERTEX || nbind==PER_VERTEX_INDEXED )
	    glNormal3fv(normals[n1].getValue());

	if ( dotextures )
	{
	    if ( tb.isFunction() )
	    {
		SbVec3f texturecoord;
		tb.get(coords->get3(v1), texturecoord );
		glTexCoord2fv( texturecoord.getValue() );
	    }
	    else
	    {
		glTexCoord2fv( tb.get(t1).getValue() );
	    }
	}

	glVertex3fv( coords->get3(v1).getValue() );
    }

    glEnd();

    if ( didpush )
	state->pop();
}


void SoDGBIndexedPointSet::generatePrimitives(SoAction* action)
{
    //if ( !coordIndex.getNum() || (coordIndex.getNum()==1 && coordIndex[0]==-1) )
	//return;

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

    if ( mbind == PER_VERTEX_INDEXED && !mindices )
	mindices = cindices;

    static const SbVec3f dummynormal(0.0f, 0.0f, 1.0f);
    const SbVec3f* currnormal = &dummynormal;
    if ( nbind==OVERALL )
    {
	if ( normals )
	    currnormal = normals;
    }

    int texidx = 0;
    int matidx = 0;
    int normidx = 0;

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
    	int32_t n1 = nindices ? *nindices++ : normidx++;
    	int32_t m1 = mindices ? *mindices++ : matidx++;
    	int32_t t1 = tindices ? *tindices++ : texidx++;

	if ( v1<0 )
	    continue;

	if ( nbind==PER_VERTEX || nbind==PER_VERTEX_INDEXED )
	{
	    pointdetail.setNormalIndex( n1 );
	    currnormal = &normals[n1];
	    vertex.setNormal( *currnormal );
	}

	if ( mbind==PER_VERTEX || mbind==PER_VERTEX_INDEXED )
	{
	    pointdetail.setMaterialIndex( m1 );
	    vertex.setMaterialIndex( m1 );
	}

	if ( dotextures )
	{
	    if ( tb.isFunction() )
		vertex.setTextureCoords(tb.get(coords->get3(v1), *currnormal));
	    else 
	    {
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


SbBool SoDGBIndexedPointSet::generateDefaultNormals( SoState* state,
						  SoNormalCache* nc)
{
    nc->set( 0, 0 );
    return true;
}
