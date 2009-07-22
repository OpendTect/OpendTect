/*+
________________________________________________________________________

  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  Author:        Kristofer
  Date:          July 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoIndexedLineSet3D.cc,v 1.13 2009-07-22 16:01:35 cvsbert Exp $";

#include "SoIndexedLineSet3D.h"

#include <Inventor/SbLinear.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoPointDetail.h>

#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoTextureCoordinateBindingElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/system/gl.h>


SO_NODE_SOURCE(SoIndexedLineSet3D);

void SoIndexedLineSet3D::initClass()
{
    SO_NODE_INIT_CLASS(SoIndexedLineSet3D, SoIndexedShape, "IndexedShape");
}


SoIndexedLineSet3D::SoIndexedLineSet3D()
{
    SO_NODE_CONSTRUCTOR(SoIndexedLineSet3D);
    SO_NODE_ADD_FIELD( radius, (5.0) );
    SO_NODE_ADD_FIELD( screenSize, (true) );
    SO_NODE_ADD_FIELD( maxRadius, (-1) );
    SO_NODE_ADD_FIELD( rightHandSystem, (true) );
}


void SoIndexedLineSet3D::GLRender(SoGLRenderAction* action)
{
    if ( !shouldGLRender(action) )
	return;

    generateTriangles(action, true );
}


void SoIndexedLineSet3D::generatePrimitives(SoAction* action)
{
    generateTriangles(action, false );
}


#define mSaveJoint(jointidx, center, vecs, dorev, jnormal ) \
    if ( world )\
    {\
	corner1[jointidx] = center+vecs[0]*scaleby;\
	corner2[jointidx] = center+vecs[1]*scaleby;\
	corner3[jointidx] = center+vecs[2]*scaleby;\
	corner4[jointidx] = center+vecs[3]*scaleby;\
    }\
    else\
    {\
	invmat.multVecMatrix( center+vecs[0]*scaleby, corner1[jointidx] );\
	invmat.multVecMatrix( center+vecs[1]*scaleby, corner2[jointidx] );\
	invmat.multVecMatrix( center+vecs[2]*scaleby, corner3[jointidx] );\
	invmat.multVecMatrix( center+vecs[3]*scaleby, corner4[jointidx] ); \
    } \
    reverse[jointidx] = dorev; \
    endnormals[jointidx] = jnormal

void SoIndexedLineSet3D::generateCoordinates( SoAction* action,
	int startindex,
	SbList<SbVec3f>& corner1, SbList<SbVec3f>& corner2,
	SbList<SbVec3f>& corner3, SbList<SbVec3f>& corner4,
	SbList<SbBool>& reverse, SbList<SbVec3f>& endnormals,
	int& nrjoints, SbBool world)
{
    const int nrindex = coordIndex.getNum();
    const int32_t* cindices = coordIndex.getValues(startindex);

    SoState* state = action->getState();
    const SoCoordinateElement* celem = SoCoordinateElement::getInstance(state);
    const int nrcoords = celem->getNum();

    const SbMatrix& mat = SoModelMatrixElement::get(state);
    SbMatrix invmat = mat.inverse();

    nrjoints = 0;
    const int32_t* stopptr = cindices+(nrindex-startindex);
    int index1 = cindices>=stopptr ? -1 : *cindices++;
    int index2 = cindices>=stopptr ? -1 : *cindices++;
    if ( index1<0 || index2<0 ) return;

    SbVec3f c1,c2;
    mat.multVecMatrix(celem->get3(index1), c1 );
    mat.multVecMatrix(celem->get3(index2), c2 );

    SbVec3f squarecoords1[4];
    if ( !getEdgeStartCoords( c1, c2, squarecoords1, action->getState() ) )
	return;

    const float rad = radius.getValue();
    const SbViewportRegion& vp = SoViewportRegionElement::get(state);
    const float nsize = rad/ float(vp.getViewportSizePixels()[1]);
    const SbViewVolume& vv = SoViewVolumeElement::get(state);

    const bool doscreensize = screenSize.getValue();
    const float maxradius = maxRadius.getValue();
    float scaleby = rad;
    if ( doscreensize )
    {
	scaleby  = rad * vv.getWorldToScreenScale(c1, nsize );
	if ( maxradius>=0 && scaleby>maxradius )
	   scaleby = maxradius; 
    }

    mSaveJoint( nrjoints, c1, squarecoords1, false, c1-c2 );
    nrjoints++;

    while ( index2>=0 )
    {
	SbVec3f c2c1 = c2-c1;
	c2c1.normalize();

	SbVec3f jointplanenormal;
	SbBool doreverse = false;;
	SbVec3f c3;
	const int index3 = cindices>=stopptr ? -1 : *cindices++;
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

	const SbPlane junctionplane( jointplanenormal, SbVec3f(0,0,0) );
	SbVec3f squarecoords2[4];
	for ( int idx=0; idx<4; idx++ )
	{
	    SbLine projline( squarecoords1[idx], squarecoords1[idx]+c2c1 );
	    junctionplane.intersect(projline,squarecoords2[idx]);
	}

	if ( doscreensize )
	{
	    scaleby  = rad * vv.getWorldToScreenScale(c2, nsize );
	    if ( maxradius>=0 && scaleby>maxradius )
	       scaleby = maxradius; 
	}

	mSaveJoint( nrjoints, c2, squarecoords2, doreverse, jointplanenormal );
	nrjoints++;

	for ( int idx=0; idx<4; idx++ )
	    squarecoords1[idx] = squarecoords2[idx];

	index1 = index2; c1 = c2;
	index2 = index3; c2 = c3;
    }
}

bool SoIndexedLineSet3D::getEdgeStartCoords( const SbVec3f& edgecoord,
		const SbVec3f& coord2, SbVec3f* res, SoState* state )
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

    res[0] = planenormal;
    res[1] = counternormal;
    res[2] = -planenormal;
    res[3] = -counternormal;

    return true;
}


void SoIndexedLineSet3D::computeBBox( SoAction* action, SbBox3f& box,
				       SbVec3f& center )
{
    const int nrcoordindex = coordIndex.getNum();
    int curindex = 0;

    SbList<SbVec3f> corner1(nrcoordindex),  corner2(nrcoordindex),
		    corner3(nrcoordindex), corner4(nrcoordindex),
		    endnormals(nrcoordindex);
    SbList<SbBool> reverse(nrcoordindex);

    bool boxinited = false;

    while ( curindex<nrcoordindex )
    {
	int nrjoints;
	generateCoordinates( action, curindex, corner1, corner2, corner3,
			     corner4, reverse, endnormals, nrjoints, false );
	if ( nrjoints<2 )
	{
	    curindex++;
	    continue;
	}

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

    if ( boxinited )
	center = box.getCenter();
}


#define mSendQuad(  c1, ci1, c2, ci2, m12, c3, ci3, c4, ci4, m34, norm23, \
       norm14	)\
    if ( render )\
    {\
	mb.send(m12,true);\
	if ( isreversed ) \
	{ \
	    glNormal3fv((norm23).getValue()); \
	    glVertex3fv((c2[ci2]).getValue());\
	    glNormal3fv((norm14).getValue()); \
	    glVertex3fv((c1[ci1]).getValue());\
	    mb.send(m34,true);\
	    glNormal3fv((norm14).getValue()); \
	    glVertex3fv((c4[ci4]).getValue());\
	    glNormal3fv((norm23).getValue()); \
	    glVertex3fv((c3[ci3]).getValue());\
	} \
	else \
	{ \
	    glNormal3fv((norm14).getValue()); \
	    glVertex3fv((c1[ci1]).getValue());\
	    glNormal3fv((norm23).getValue()); \
	    glVertex3fv((c2[ci2]).getValue());\
	    mb.send(m34,true);\
	    glNormal3fv((norm23).getValue()); \
	    glVertex3fv((c3[ci3]).getValue());\
	    glNormal3fv((norm14).getValue()); \
	    glVertex3fv((c4[ci4]).getValue());\
	} \
    }\
    else\
    {\
	pv.setNormal(norm14); \
	pv.setMaterialIndex(m12);\
	pv.setPoint(c1[ci1]);\
	pointdetail.setCoordinateIndex(ci1); \
	shapeVertex(&pv);\
	pv.setNormal(norm23); \
	pv.setPoint(c2[ci2]);\
	pointdetail.setCoordinateIndex(ci2); \
	shapeVertex(&pv);\
	pv.setNormal(norm14); \
	pv.setMaterialIndex(m34);\
	pv.setPoint(c4[ci4]);\
	pointdetail.setCoordinateIndex(ci4); \
	shapeVertex(&pv);\
	pv.setNormal(norm23); \
	facedetail.incFaceIndex(); \
	pv.setPoint(c3[ci3]);\
	pointdetail.setCoordinateIndex(ci3); \
	shapeVertex(&pv);\
	facedetail.incFaceIndex(); \
    }


void SoIndexedLineSet3D::generateTriangles( SoAction* action, bool render )
{
    const int nrcoordindex = coordIndex.getNum();
    int curindex = 0;

    SoState* state = action->getState();
    SoTextureCoordinateBundle tb(action, render, render);
    SoMaterialBundle mb(action);

    SbList<SbVec3f> corner1(nrcoordindex),  corner2(nrcoordindex),
		    corner3(nrcoordindex), corner4(nrcoordindex),
		    endnormals(nrcoordindex);
    SbList<SbBool> reverse(nrcoordindex);

    int matnr = 0;
    SoMaterialBindingElement::Binding mbind =
				SoMaterialBindingElement::get(state);

    SoPrimitiveVertex pv;
    SoFaceDetail facedetail;
    SoPointDetail pointdetail;
    pv.setDetail(&pointdetail);


    while ( curindex<nrcoordindex )
    {
	int nrjoints;
	generateCoordinates( action, curindex, corner1, corner2, corner3,
			     corner4, reverse, endnormals, nrjoints, render );
	if ( nrjoints<2 )
	    break;

	if ( render )
	{
	    glPushMatrix();
	    SbMatrix m = SoViewingMatrixElement::get(state);
	    glLoadMatrixf(m[0]);
	    mb.sendFirst();
	    glBegin(GL_QUADS);
	}
	else
	{
	    facedetail.setFaceIndex(0);
	    beginShape(action,TRIANGLE_STRIP, &facedetail );
	}

	const int32_t* cis = coordIndex.getValues(curindex);
	const int32_t* materialindexes = materialIndex.getValues(curindex);
	if ( !materialindexes &&
		mbind==SoMaterialBindingElement::PER_VERTEX_INDEXED )
	    materialindexes = cis;

	int material1 = 0;
	if ( mbind==SoMaterialBindingElement::PER_PART ||
	     mbind==SoMaterialBindingElement::PER_FACE ||
	     mbind==SoMaterialBindingElement::PER_VERTEX )
	    material1 = matnr++;
	else if ( mbind==SoMaterialBindingElement::PER_PART_INDEXED ||
		  mbind==SoMaterialBindingElement::PER_FACE_INDEXED ||
		  mbind==SoMaterialBindingElement::PER_VERTEX_INDEXED )
	    material1 = materialindexes[0];

	bool isreversed = rightHandSystem.getValue();
	mSendQuad(  corner1, 0, corner2, 0, material1,
		    corner3, 0, corner4, 0, material1,
		    endnormals[0], endnormals[0] );

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

	    const SbVec3f face0normal = (isreversed ? 1 : -1 ) *
				 (corner2[idx]-corner1[idx]).cross(
				 corner1[idx+1]-corner1[idx] );
	    const SbVec3f face1normal = (isreversed ? 1 : -1 ) *
				 (corner3[idx]-corner2[idx]).cross(
				 corner2[idx+1]-corner2[idx] );
	    const SbVec3f face2normal = (isreversed ? 1 : -1 ) *
				 (corner4[idx]-corner3[idx]).cross(
				 corner3[idx+1]-corner3[idx] );
	    const SbVec3f face3normal = (isreversed ? 1 : -1 ) *
				 (corner1[idx]-corner4[idx]).cross(
				 corner4[idx+1]-corner4[idx] );

	    SbList<SbVec3f> cornernormals;
	    cornernormals.append(face3normal+face0normal);
	    cornernormals.append(face0normal+face1normal);
	    cornernormals.append(face1normal+face2normal);
	    cornernormals.append(face2normal+face3normal);

	    mSendQuad(  corner2, idx, corner1, idx, material1,
			corner1, idx+1, corner2, idx+1, material2,
			cornernormals[0], cornernormals[1] );

	    mSendQuad(  corner3, idx, corner2, idx, material1,
			corner2, idx+1, corner3, idx+1, material2,
			cornernormals[1], cornernormals[2] );

	    mSendQuad(  corner4, idx, corner3, idx, material1,
			corner3, idx+1, corner4, idx+1, material2, 
		   	cornernormals[2], cornernormals[3] );

	    mSendQuad(  corner1, idx, corner4, idx, material1,
			corner4, idx+1, corner1, idx+1, material2, 
		   	cornernormals[3], cornernormals[0] );

	    if ( reverse[idx+1] || idx==nrjoints-2)
	    {
		isreversed = !isreversed;
		mSendQuad(  corner1, idx+1, corner2, idx+1, material2,
			    corner3, idx+1, corner4, idx+1, material2,
			    endnormals[idx+1], endnormals[idx+1] );
	    }

	    material1 = material2;
	}

	if ( render )
	{
	    glEnd();
	    glPopMatrix();
	}
	else
	{
	    endShape();
	    facedetail.incPartIndex();
	}

	curindex += nrjoints+1;
    }
}
