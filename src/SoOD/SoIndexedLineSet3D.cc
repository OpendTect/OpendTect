/*+
________________________________________________________________________

  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  Author:        Kristofer
  Date:          July 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "SoIndexedLineSet3D.h"

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

SO_NODE_SOURCE(SoIndexedLineSet3D);

void SoIndexedLineSet3D::initClass()
{
    SO_NODE_INIT_CLASS(SoIndexedLineSet3D, SoIndexedShape, "IndexedShape");
    SO_ENABLE( SoGLRenderAction, SoCameraInfoElement );
    SO_ENABLE( SoGLRenderAction, SoModelMatrixElement );
    SO_ENABLE( SoGLRenderAction, SoViewportRegionElement );
    SO_ENABLE( SoGLRenderAction, SoViewVolumeElement );
    SO_ENABLE( SoGLRenderAction, SoCacheElement );
}


SoIndexedLineSet3D::LineSet3DData::LineSet3DData()
    : nodeid_( -1 )
    , modelmatchinfo_( 0 )
    , coordmatchinfo_( 0 )
    , vvmatchinfo_( 0 )
    , vpmatchinfo_( 0 )
#ifdef USE_DISPLAYLIST_LINESET
    , displaylist_( 0 )
#endif
{}


SoIndexedLineSet3D::LineSet3DData::~LineSet3DData()
{
    delete modelmatchinfo_;
    delete coordmatchinfo_;
    delete vvmatchinfo_;
    delete vpmatchinfo_;
#ifdef USE_DISPLAYLIST_LINESET
    displaylist_->unref();
#endif
}


SoIndexedLineSet3D::SoIndexedLineSet3D()
{
    SO_NODE_CONSTRUCTOR(SoIndexedLineSet3D);
    SO_NODE_ADD_FIELD( radius, (5.0) );
    SO_NODE_ADD_FIELD( screenSize, (true) );
    SO_NODE_ADD_FIELD( maxRadius, (-1) );
}


SoIndexedLineSet3D::~SoIndexedLineSet3D()
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


void SoIndexedLineSet3D::GLRender(SoGLRenderAction* action)
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
	SoDebugError::postWarning( "SoIndexedLineSet3D::GLRender",
	       "Cannot create display list!" );
	return;
    }

    if ( SoCacheElement::anyOpen( state ) )
    {
	SoDebugError::postWarning( "SoIndexedLineSet3D::GLRender",
 	"A cache is already open! Cannot generate coordinates now!" );
	return;
    }
#endif

    bool isvalid = data_.areCoordsValid( state, this, screenSize.getValue() );

    if ( !isvalid )
    {
	data_.generateCoordinates( this, radius.getValue(),
		screenSize.getValue(), maxRadius.getValue(),
		coordIndex.getValues(0), coordIndex.getNum(), state );
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

    SoMaterialBindingElement::Binding mbind =
				SoMaterialBindingElement::get(state);

    const int32_t* materialindexes = materialIndex.getValues(0);
    if ( !materialindexes &&
	    mbind==SoMaterialBindingElement::PER_VERTEX_INDEXED )
	materialindexes = coordIndex.getValues(0);

    data_.glRender( materialindexes, action );
}


void SoIndexedLineSet3D::LineSet3DData::glRender(
	const int32_t* materialindexes, SoGLRenderAction* action )
{
    SoState* state = action->getState();
    SoTextureCoordinateBundle tb(action, true, true);
    SoMaterialBundle mb(action);
    int matnr = 0;
    SoMaterialBindingElement::Binding mbind =
				SoMaterialBindingElement::get(state);

    glPushMatrix();
    SbMatrix m = SoViewingMatrixElement::get(state);
    glLoadMatrixf(m[0]);

    for ( int idx=0; idx<sectionstarts_.getLength(); idx++ )
    {
	bool isreversed = true;

	const int start = sectionstarts_[idx];
	const int stop = idx==sectionstarts_.getLength()-1
	    ? corner1_.getLength()-1
	    : sectionstarts_[idx+1]-1;

	mb.sendFirst();
	glBegin(GL_QUADS);

	int material1 = 0;
	if ( mbind==SoMaterialBindingElement::PER_PART ||
	     mbind==SoMaterialBindingElement::PER_FACE ||
	     mbind==SoMaterialBindingElement::PER_VERTEX )
	    material1 = matnr++;
	else if ( mbind==SoMaterialBindingElement::PER_PART_INDEXED ||
		  mbind==SoMaterialBindingElement::PER_FACE_INDEXED ||
		  mbind==SoMaterialBindingElement::PER_VERTEX_INDEXED )
	    material1 = materialindexes[matnr++];

	mRenderQuad( corner1_[start], corner2_[start], material1,
		    corner3_[start], corner4_[start], material1,
		    endnormals_[idx], endnormals_[idx] );

	for ( int idy=start; idy<stop; idy++ )
	{
	    int material2 = 0;
	    if ( mbind==SoMaterialBindingElement::PER_PART ||
		 mbind==SoMaterialBindingElement::PER_FACE ||
		 mbind==SoMaterialBindingElement::PER_VERTEX )
		material2 = matnr++;
	    else if ( mbind==SoMaterialBindingElement::PER_PART_INDEXED ||
		      mbind==SoMaterialBindingElement::PER_FACE_INDEXED ||
		      mbind==SoMaterialBindingElement::PER_VERTEX_INDEXED )
		material2 = materialindexes[matnr++]; //wher get matnr

	    mRenderQuad(  corner2_[idy], corner1_[idy], material1,
			corner1_[idy+1], corner2_[idy+1], material2,
			cornernormal1_[idy], cornernormal2_[idy+1] );

	    mRenderQuad(  corner3_[idy], corner2_[idy], material1,
			corner2_[idy+1], corner3_[idy+1], material2,
			cornernormal2_[idy], cornernormal3_[idy+1] );

	    mRenderQuad(  corner4_[idy], corner3_[idy], material1,
			corner3_[idy+1], corner4_[idy+1], material2, 
		   	cornernormal3_[idy], cornernormal4_[idy+1] );

	    mRenderQuad(  corner1_[idy], corner4_[idy], material1,
			corner4_[idy+1], corner1_[idy+1], material2, 
		   	cornernormal4_[idy], cornernormal1_[idy+1] );

	    if ( isreversed_[idy+1] || idy==stop-1 )
	    {
		isreversed = !isreversed;
		mRenderQuad(  corner1_[idy+1], corner2_[idy+1], material2,
			    corner3_[idy+1], corner4_[idy+1], material2,
			    endnormals_[idy+1], endnormals_[idy+1] );
	    }

	    material1 = material2;
	}

	matnr++; //compesate for -1

	glEnd();
    }

    glPopMatrix();

#ifdef USE_DISPLAYLIST_LINESET
    displaylist_->close( state );
#endif
}


#define mSaveJoint( center, vecs, dorev, jnormal ) \
    corner1_.append( center+vecs[0]*scaleby );\
    corner2_.append( center+vecs[1]*scaleby );\
    corner3_.append( center+vecs[2]*scaleby );\
    corner4_.append( center+vecs[3]*scaleby );\
    cornernormal1_.append( vecs[0] ); \
    cornernormal2_.append( vecs[1] ); \
    cornernormal3_.append( vecs[2] ); \
    cornernormal4_.append( vecs[3] ); \
    isreversed_.append( dorev ); \
    endnormals_.append( jnormal ); \


bool SoIndexedLineSet3D::LineSet3DData::getEdgeStartCoords(
	const SbVec3f& edgecoord,
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


bool SoIndexedLineSet3D::LineSet3DData::areCoordsValid( SoState* state,
						    SoNode* node,
						    bool doscreensize ) const
{
    if ( nodeid_!=node->getNodeId() )
	return false;

#define mCheckElem( var, elem ) \
    if ( var && \
	 !var->matches(state->getConstElement( elem::getClassStackIndex() )) ) \
	    return false;

    mCheckElem( coordmatchinfo_, SoCoordinateElement );

    if ( doscreensize )
    {
	const int32_t camerainfo = SoCameraInfoElement::get(state);
	if ( !(camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE)) )
	{
	    mCheckElem( modelmatchinfo_, SoModelMatrixElement );
	    mCheckElem( vpmatchinfo_, SoViewportRegionElement );
	    mCheckElem( vvmatchinfo_, SoViewVolumeElement );
	    SoCacheElement::invalidate( state );
	}
    }

    return true;
}


void SoIndexedLineSet3D::generatePrimitives(SoAction* action)
{
}


void SoIndexedLineSet3D::rayPick( SoRayPickAction* action )
{
    SoState* state = action->getState();
    SbBox3f box;
    SbVec3f dummy;

    computeBBox( action, box, dummy );
    if ( !action->intersect(box,dummy) )
	return;

    const int nrindex = coordIndex.getNum();
    if ( !nrindex ) 
	return;

    const int32_t* cindices = coordIndex.getValues(0);

    const SoCoordinateElement* celem = SoCoordinateElement::getInstance(state);
    const int nrcoords = celem->getNum();
    const int32_t* stopptr = cindices + nrindex;

    while ( cindices<stopptr )
    {
	int index1 = *cindices++;
	if ( index1<0 || index1>=nrcoords )
	    continue;

	SbVec3f c1 = celem->get3(index1);
	while ( index1>=0 )
	{
	    const int index2 = *cindices++;
	    if ( index2<0 || index2>=nrcoords )
		break;

	    const SbVec3f c2 = celem->get3( index2 );

	    if ( action->intersect( c1, c2, dummy ) )
	    {
		SoPickedPoint* pickedpoint mUnusedVar =
		    action->addIntersection(dummy);
		//Todo: Fill out pickedpoint
	    }

	    if ( cindices>=stopptr )
		break;

	    c1 = c2; index1 = index2;
	}
    }
}


void SoIndexedLineSet3D::LineSet3DData::generateCoordinates( SoNode* node,
	float theradius, bool doscreensize, float maxradius, 
	const int* cindices, int nrindex, SoState* state )
{
    corner1_.truncate( 0, 0 );
    corner2_.truncate( 0, 0 );
    corner3_.truncate( 0, 0 );
    corner4_.truncate( 0, 0 );
    cornernormal1_.truncate( 0, 0 );
    cornernormal2_.truncate( 0, 0 );
    cornernormal3_.truncate( 0, 0 );
    cornernormal4_.truncate( 0, 0 );
    endnormals_.truncate( 0, 0 );
    isreversed_.truncate( 0, 0 );
    sectionstarts_.truncate( 0, 0 );

    corner1_.ensureCapacity( nrindex );
    corner2_.ensureCapacity( nrindex );
    corner3_.ensureCapacity( nrindex );
    corner4_.ensureCapacity( nrindex );
    cornernormal1_.ensureCapacity( nrindex );
    cornernormal2_.ensureCapacity( nrindex );
    cornernormal3_.ensureCapacity( nrindex );
    cornernormal4_.ensureCapacity( nrindex );
    endnormals_.ensureCapacity( nrindex );
    isreversed_.ensureCapacity( nrindex );

    const SoCoordinateElement* celem = SoCoordinateElement::getInstance(state);
    const int nrcoords = celem->getNum();
    const int32_t* stopptr = cindices + nrindex;

    const SbMatrix& mat = SoModelMatrixElement::get(state);

    const SbViewportRegion& vp = SoViewportRegionElement::get(state);
    const float nsize = theradius/ float(vp.getViewportSizePixels()[1]);
    const SbViewVolume& vv = SoViewVolumeElement::get(state);

    float scaleby = theradius;

    int nrjoints = 0;
    int index1 = cindices>=stopptr ? -1 : *cindices++;
    while ( index1>=0 && index1<nrcoords )
    {
	int index2 = cindices>=stopptr ? -1 : *cindices++;
	if ( index2<0 || index2>=nrcoords )
	    break;

	SbVec3f c1,c2;
	mat.multVecMatrix( celem->get3(index1), c1 );
	mat.multVecMatrix( celem->get3(index2), c2 );

	SbVec3f squarecoords1[4];
	if ( !getEdgeStartCoords( c1, c2, squarecoords1, state ) )
	    return;

	if ( doscreensize )
	{
	    scaleby  = theradius * vv.getWorldToScreenScale(c1, nsize );
	    if ( maxradius>=0 && scaleby>maxradius )
	       scaleby = maxradius; 
	}

	mSaveJoint( c1, squarecoords1, false, c1-c2 );
	sectionstarts_.append( nrjoints++ );

	while ( index2>=0 )
	{
	    SbVec3f c2c1 = c2-c1;
	    c2c1.normalize();

	    SbVec3f jointplanenormal;
	    SbBool doreverse = false;
	    SbVec3f c3;
	    const int index3 = cindices>=stopptr ? -1 : *cindices++;
	    if ( index3>=0 && index3<nrcoords )
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
		scaleby  = theradius * vv.getWorldToScreenScale(c2, nsize );
		if ( maxradius>=0 && scaleby>maxradius )
		   scaleby = maxradius; 
	    }

	    mSaveJoint(c2,squarecoords2,doreverse,jointplanenormal);
	    nrjoints++;

	    for ( int idx=0; idx<4; idx++ )
		squarecoords1[idx] = squarecoords2[idx];

	    index1 = index2; c1 = c2;
	    index2 = index3; c2 = c3;
	}

	index1 = cindices>=stopptr ? -1 : *cindices++;
    }

    delete coordmatchinfo_; coordmatchinfo_ = celem->copyMatchInfo();

#define mCopyMatchInfo( var, elem ) \
    delete var; \
    var = state->getConstElement( elem::getClassStackIndex() )->copyMatchInfo();

    mCopyMatchInfo( modelmatchinfo_, SoModelMatrixElement );
    mCopyMatchInfo( vpmatchinfo_, SoViewportRegionElement );
    mCopyMatchInfo( vvmatchinfo_, SoViewVolumeElement );

    nodeid_ = node->getNodeId();
}


/*
void SoIndexedLineSet3D::computeBBox( SoAction* action, SbBox3f& box,
				      SbVec3f& center )
{
    SoState* state = action->getState();
    if ( !areCoordsValid(state) )
	generateCoordinates( state );

    box = bbox_;
    center = center_;
}
*/


/*
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

	bool isreversed = true;
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

*/
