#include "SoIndexedPolyLine3D.h"

#include <Inventor/SbLinear.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/bundles/SoMaterialBundle.h>

#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoTextureCoordinateBindingElement.h>

#ifdef mac
# include "OpenGL/gl.h"
#else
# include "GL/gl.h"
#endif



SO_NODE_SOURCE(SoIndexedPolyLine3D);

void SoIndexedPolyLine3D::initClass()
{
    SO_NODE_INIT_CLASS(SoIndexedPolyLine3D, SoIndexedShape, "IndexedShape");
}


SoIndexedPolyLine3D::SoIndexedPolyLine3D()
{
    SO_NODE_CONSTRUCTOR(SoIndexedPolyLine3D);
    SO_NODE_ADD_FIELD( radius, (5.0) );
}


void SoIndexedPolyLine3D::GLRender(SoGLRenderAction* action)
{
    generateTriangles(action, true );
}


void SoIndexedPolyLine3D::generatePrimitives(SoAction* action)
{
    generateTriangles(action, false );

/*
    if ( coordIndex.getNum()<2 )
	return;

    SoState* state = action->getState();
    SoMaterialBindingElement::Binding mbind =
				SoMaterialBindingElement::get(state);

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

    getVertexData(  state, coords, normals, cindices,
		    nindices, tindices, mindices, numindices,
		    sendnormals, normalcacheused );

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

    if (mbind == SoMaterialBindingElement::PER_VERTEX_INDEXED && !mindices )
	mindices = cindices;

    int texidx = 0;
    int matnr = 0;

    const int32_t* viptr = cindices;
    const int32_t* viendptr = viptr + numindices;
    SoPrimitiveVertex pv;

    while ( viptr+2<viendptr )
    {
	bool isreversed = false;
	int32_t v1 = *viptr++;
	SbVec3f c1 = coords->get3(v1);
	int m1 = 0, m2 = 0;

	if ( mbind==SoMaterialBindingElement::PER_PART ||
	     mbind==SoMaterialBindingElement::PER_FACE ||
	     mbind==SoMaterialBindingElement::PER_VERTEX )
	{
	    m1 = matnr++;
	    m2 = matnr;
	}
	else if ( mbind==SoMaterialBindingElement::PER_PART_INDEXED ||
		  mbind==SoMaterialBindingElement::PER_FACE_INDEXED ||
		  mbind==SoMaterialBindingElement::PER_VERTEX_INDEXED )
	{
	    m1 = *mindices++;
	    m2 = *mindices;
	}

	int32_t v2 = *viptr;
	SbVec3f c2 = coords->get3(v2);

	SbVec3f squarecoords1[4];
	if ( !getEdgeStartCoords( c1, c2, squarecoords1) )
	    continue;

	beginShape( action, TRIANGLES );
	pv.setNormal(c1-c2);
	pv.setMaterialIndex(m1);
	pv.setPoint(squarecoords1[0]);
	shapeVertex(&pv);
	pv.setPoint(squarecoords1[1]);
	shapeVertex(&pv);
	pv.setPoint(squarecoords1[3]);
	shapeVertex(&pv);

	pv.setPoint(squarecoords1[3]);
	shapeVertex(&pv);
	pv.setPoint(squarecoords1[1]);
	shapeVertex(&pv);
	pv.setPoint(squarecoords1[2]);
	shapeVertex(&pv);

	endShape();

	while ( v2>=0 )
	{
	    viptr++;
	    mindices++;
	    const int32_t v3 = *viptr;
	    SbVec3f c3;

	    SbVec3f c2c1 = c2-c1;
	    c2c1.normalize();
	    SbPlane junctionplane;
	    bool doreverse = false;
	    if ( v3>=0 )
	    {
		c3 = coords->get3(v3);
		SbVec3f c3c2 = c3-c2;
		c3c2.normalize();
		doreverse = c2c1.dot(c3c2)<-0.5;
		SbVec3f planenormal = doreverse ? c2c1-c3c2 :c2c1+c3c2;
		if ( !planenormal.length() )
		    planenormal = c2c1;


		junctionplane = SbPlane( planenormal, c2 );
	    }
	    else
		junctionplane = SbPlane( c2c1, c2 );

	    SbVec3f squarecoords2[4];
	    for ( int idx=0; idx<4; idx++ )
	    {
		SbLine projline( squarecoords1[idx], squarecoords1[idx]+c2c1 );
		junctionplane.intersect(projline,squarecoords2[idx]);
	    }

	    beginShape( action, TRIANGLES );
	    for ( int idx=0; idx<4; idx++ )
	    {
		SbVec3f normal = (isreversed ? -1 : 1 ) *
		    (squarecoords1[(idx+1)%4]-squarecoords1[idx]).cross
		    (squarecoords2[idx]-squarecoords1[idx]);

		pv.setNormal(normal);

		pv.setPoint(squarecoords1[idx]);
		pv.setMaterialIndex(m1);
		shapeVertex(&pv);
		pv.setPoint(squarecoords2[idx]);
		pv.setMaterialIndex(m2);
		shapeVertex(&pv);
		pv.setPoint(squarecoords1[(idx+1)%4]);
		pv.setMaterialIndex(m1);
		shapeVertex(&pv);

		shapeVertex(&pv);
		pv.setPoint(squarecoords2[idx]);
		pv.setMaterialIndex(m2);
		shapeVertex(&pv);
		pv.setPoint(squarecoords2[(idx+1)%4]);
		shapeVertex(&pv);
	    }

	    endShape();


	    if ( v3<0 || doreverse )
	    {
		beginShape( action, TRIANGLES );
		pv.setMaterialIndex(m2);
		pv.setNormal(c2-c1);
		pv.setPoint(squarecoords2[0]);
		shapeVertex(&pv);
		pv.setPoint(squarecoords2[1]);
		shapeVertex(&pv);
		pv.setPoint(squarecoords2[3]);
		shapeVertex(&pv);

		pv.setPoint(squarecoords2[3]);
		shapeVertex(&pv);
		pv.setPoint(squarecoords2[1]);
		shapeVertex(&pv);
		pv.setPoint(squarecoords2[2]);
		shapeVertex(&pv);

		endShape();
	    }

	    if ( doreverse ) isreversed = !isreversed;

	    if ( v3<0 )
	    {
		viptr++;
		++mindices;
		break;
	    }

	    for ( int idx=0; idx<4; idx++ )
		squarecoords1[idx] = squarecoords2[idx];

	    v1 = v2; c1 = c2;
	    v2 = v3; c2 = c3;
	    m1 = m2;
	    if ( mbind==SoMaterialBindingElement::PER_PART ||
		 mbind==SoMaterialBindingElement::PER_FACE ||
		 mbind==SoMaterialBindingElement::PER_VERTEX )
	    {
		m2 = ++matnr;
	    }
	    else if ( mbind==SoMaterialBindingElement::PER_PART_INDEXED ||
		      mbind==SoMaterialBindingElement::PER_FACE_INDEXED ||
		      mbind==SoMaterialBindingElement::PER_VERTEX_INDEXED )
	    {
		m2 = *mindices;
	    }
	}
    }
*/
}


#define mSaveJoint(jointidx, coords, dorev, jnormal ) \
    if ( world )\
    {\
	corner1[jointidx] = coords[0];\
	corner2[jointidx] = coords[1];\
	corner3[jointidx] = coords[2];\
	corner4[jointidx] = coords[3];\
    }\
    else\
    {\
	invmat.multVecMatrix( coords[0], corner1[jointidx] );\
	invmat.multVecMatrix( coords[1], corner2[jointidx] );\
	invmat.multVecMatrix( coords[2], corner3[jointidx] );\
	invmat.multVecMatrix( coords[3], corner4[jointidx] ); \
    } \
    reverse[jointidx] = dorev; \
    endnormals[jointidx] = jnormal

void SoIndexedPolyLine3D::generateCoordinates( SoAction* action,
	int startindex,
	SbVec3f* corner1, SbVec3f* corner2, SbVec3f* corner3, SbVec3f* corner4,
	SbBool* reverse, SbVec3f* endnormals, int& nrjoints, SbBool world)
{
    const int nrindex = coordIndex.getNum();
    const int32_t* cindices = coordIndex.getValues(startindex);

    SoState* state = action->getState();
    const SoCoordinateElement* celem = SoCoordinateElement::getInstance(state);
    const int nrcoords = celem->getNum();

    const SbMatrix& mat = SoModelMatrixElement::get(state);
    SbMatrix invmat = mat.inverse();

    nrjoints = 0;
    int index1 = *cindices++;
    int index2 = *cindices++;
    if ( index1<0 || index2<0 || index1>=nrcoords || index2>nrcoords ) return;

    SbVec3f c1,c2;
    mat.multVecMatrix(celem->get3(index1), c1 );
    mat.multVecMatrix(celem->get3(index2), c2 );

    SbVec3f squarecoords1[4];
    if ( !getEdgeStartCoords( c1, c2, squarecoords1) )
	return;

    mSaveJoint( nrjoints, squarecoords1, false, c1-c2 );
    nrjoints++;

    while ( index2>=0 )
    {
	SbVec3f c2c1 = c2-c1;
	c2c1.normalize();

	SbVec3f jointplanenormal;
	SbBool doreverse = false;;
	SbVec3f c3;
	const int index3 = *cindices++;
	if ( index3>=0 )
	{
	    mat.multVecMatrix(celem->get3(index3), c3 );
	    SbVec3f c3c2 = c3-c2; c3c2.normalize();

	    doreverse = c2c1.dot(c3c2)<-0.5;
	    jointplanenormal = doreverse ? c2c1-c3c2 :c2c1+c3c2;
	    if ( !jointplanenormal.length() )
		jointplanenormal = c2c1;
	}
	else
	    jointplanenormal = c2c1;

	SbPlane junctionplane = SbPlane( jointplanenormal, c2 );
	SbVec3f squarecoords2[4];
	for ( int idx=0; idx<4; idx++ )
	{
	    SbLine projline( squarecoords1[idx], squarecoords1[idx]+c2c1 );
	    junctionplane.intersect(projline,squarecoords2[idx]);
	}

	mSaveJoint( nrjoints, squarecoords2, doreverse, jointplanenormal );
	nrjoints++;

	for ( int idx=0; idx<4; idx++ )
	    squarecoords1[idx] = squarecoords2[idx];

	index1 = index2; c1 = c2;
	index2 = index3; c2 = c3;
    }
}

bool SoIndexedPolyLine3D::getEdgeStartCoords( const SbVec3f& edgecoord,
		const SbVec3f& coord2, SbVec3f* res )
{
    if ( edgecoord==coord2 ) return false;

    SbVec3f planenormal;
    const SbLine line(edgecoord, coord2);
    bool found = false;
    for ( int idx=-1; !found && idx<=1; idx++ )
    {
	for ( int idy=-1; idy<=1; idy++ )
	{
	    const SbVec3f dummy( idx, idy, 0 );
	    const SbVec3f closest = line.getClosestPoint(dummy);
	    if ( (dummy-closest).length()>0.5 )
	    {
		planenormal = dummy-closest;
		planenormal.normalize();
		found = true;
		break;
	    }
	}
    }

    if ( !found ) return false;

    const SbVec3f counternormal = line.getDirection().cross(planenormal);

    float rad = radius.getValue();
    res[0] = edgecoord-planenormal*rad;
    res[1] = edgecoord+counternormal*rad;
    res[2] = edgecoord+planenormal*rad;
    res[3] = edgecoord-counternormal*rad;
    return true;
}


void SoIndexedPolyLine3D::computeBBox( SoAction* action, SbBox3f& box,
				       SbVec3f& center )
{
    const int nrcoordindex = coordIndex.getNum();
    int curindex = 0;

    SbVec3f corner1[nrcoordindex],  corner2[nrcoordindex],
            corner3[nrcoordindex], corner4[nrcoordindex],
	    endnormals[nrcoordindex];
    SbBool reverse[nrcoordindex];

    bool boxinited = false;

    while ( curindex<nrcoordindex )
    {
	int nrjoints;
	generateCoordinates( action, curindex, corner1, corner2, corner3,
			     corner4, reverse, endnormals, nrjoints, false );
	if ( nrjoints<2 )
	    continue;

	for ( int idx=0; idx<nrjoints; idx++ )
	{
	    if ( !boxinited )
	    {
		boxinited = true;
		box.getMin() = corner1[idx];
		box.getMax() = corner1[idx];
	    }
	    else
		box.extendBy(corner1[idx]);

	    box.extendBy(corner2[idx]);
	    box.extendBy(corner3[idx]);
	    box.extendBy(corner4[idx]);
	}

	curindex += nrjoints+1;
    }

    center = box.getCenter();
}


#define mSendQuad(  c1, c2, m12, c3, c4, m34, norm )\
    if ( render )\
    {\
	mb.send(m12,true);\
	glNormal3fv((norm).getValue()); \
	if ( isreversed ) \
	{ \
	    glVertex3fv((c2).getValue());\
	    glVertex3fv((c1).getValue());\
	    mb.send(m34,true);\
	    glNormal3fv((norm).getValue()); \
	    glVertex3fv((c4).getValue());\
	    glVertex3fv((c3).getValue());\
	} \
	else \
	{ \
	    glVertex3fv((c1).getValue());\
	    glVertex3fv((c2).getValue());\
	    mb.send(m34,true);\
	    glNormal3fv((norm).getValue()); \
	    glVertex3fv((c3).getValue());\
	    glVertex3fv((c4).getValue());\
	} \
    }\
    else\
    {\
	pv.setNormal(norm); \
	pv.setMaterialIndex(m12);\
	pv.setPoint(c1);\
	shapeVertex(&pv);\
	pv.setPoint(c2);\
	shapeVertex(&pv);\
	pv.setMaterialIndex(m34);\
	pv.setPoint(c4);\
	shapeVertex(&pv);\
	pv.setPoint(c3);\
	shapeVertex(&pv);\
    }


void SoIndexedPolyLine3D::generateTriangles( SoAction* action, bool render )
{
    const int nrcoordindex = coordIndex.getNum();
    int curindex = 0;

    SoState* state = action->getState();
    SoTextureCoordinateBundle tb(action, render, render);
    SoMaterialBundle mb(action);

    SoPrimitiveVertex pv;

    SbVec3f corner1[nrcoordindex],  corner2[nrcoordindex],
            corner3[nrcoordindex], corner4[nrcoordindex],
	    endnormals[nrcoordindex];
    SbBool reverse[nrcoordindex];

    int matnr = 0;
    SoMaterialBindingElement::Binding mbind =
				SoMaterialBindingElement::get(state);

    while ( curindex<nrcoordindex )
    {
	int nrjoints;
	generateCoordinates( action, curindex, corner1, corner2, corner3,
			     corner4, reverse, endnormals, nrjoints, false );
	if ( nrjoints<2 )
	    break;

	if ( render )
	{
	    beginSolidShape(reinterpret_cast<SoGLRenderAction*>(action));
	    glPushMatrix();
	    //glLoadIdentity();
	    mb.sendFirst();
	    glBegin(GL_QUADS);
	}
	else
	{
	    beginShape(action,TRIANGLE_STRIP );
	}

	const int32_t* materialindexes = materialIndex.getValues(curindex);
	if ( !materialindexes &&
		mbind==SoMaterialBindingElement::PER_VERTEX_INDEXED )
	    materialindexes = coordIndex.getValues(curindex);

	int material1 = 0;
	if ( mbind==SoMaterialBindingElement::PER_PART ||
	     mbind==SoMaterialBindingElement::PER_FACE ||
	     mbind==SoMaterialBindingElement::PER_VERTEX )
	    material1 = matnr++;
	else if ( mbind==SoMaterialBindingElement::PER_PART_INDEXED ||
		  mbind==SoMaterialBindingElement::PER_FACE_INDEXED ||
		  mbind==SoMaterialBindingElement::PER_VERTEX_INDEXED )
	    material1 = materialindexes[0];

	bool isreversed = false;
	mSendQuad(  corner1[0], corner2[0], material1,
		    corner3[0], corner4[0], material1, endnormals[0] );

	for ( int idx=0; idx<nrjoints-1; idx++ )
	{
	    int material2 = 0;
	    if ( mbind==SoMaterialBindingElement::PER_PART ||
		 mbind==SoMaterialBindingElement::PER_FACE ||
		 mbind==SoMaterialBindingElement::PER_VERTEX )
		material2 = matnr++;
	    else if ( mbind==SoMaterialBindingElement::PER_PART_INDEXED ||
		      mbind==SoMaterialBindingElement::PER_FACE_INDEXED ||
		      mbind==SoMaterialBindingElement::PER_VERTEX_INDEXED )
		material2 = materialindexes[idx+1];

	    SbVec3f normal = (isreversed ? 1 : -1 ) *
				 (corner2[idx]-corner1[idx]).cross(
				 corner1[idx+1]-corner1[idx] );
	    mSendQuad(  corner2[idx], corner1[idx], material1,
			corner1[idx+1], corner2[idx+1], material2, normal );

	    normal = (isreversed ? 1 : -1 ) *
				 (corner3[idx]-corner2[idx]).cross(
				 corner2[idx+1]-corner2[idx] );
	    mSendQuad(  corner3[idx], corner2[idx], material1,
			corner2[idx+1], corner3[idx+1], material2, normal );

	    normal = (isreversed ? 1 : -1 ) *
				 (corner4[idx]-corner3[idx]).cross(
				 corner3[idx+1]-corner3[idx] );
	    mSendQuad(  corner4[idx], corner3[idx], material1,
			corner3[idx+1], corner4[idx+1], material2, normal );

	    normal = (isreversed ? 1 : -1 ) *
				 (corner1[idx]-corner4[idx]).cross(
				 corner4[idx+1]-corner4[idx] );
	    mSendQuad(  corner1[idx], corner4[idx], material1,
			corner4[idx+1], corner1[idx+1], material2, normal );

	    if ( reverse[idx+1] || idx==nrjoints-2)
	    {
		isreversed = !isreversed;
		mSendQuad(  corner1[idx+1], corner2[idx+1], material2,
			    corner3[idx+1], corner4[idx+1], material2,
			    endnormals[idx+1] );
	    }

	    material1 = material2;
	}

	if ( render )
	{
	    glEnd();
	    endSolidShape(reinterpret_cast<SoGLRenderAction*>(action));
	    glPopMatrix();
	}
	else
	{
	    endShape();
	}

	curindex += nrjoints+1;
    }
}
