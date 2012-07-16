/*
___________________________________________________________________
/
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Karthika
 * DATE     : July 2009
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: SoBeachBall.cc,v 1.13 2012-07-16 20:57:25 cvskris Exp $";

#include "SoBeachBall.h"
#include "SoCameraInfoElement.h"
#include "SoCameraInfo.h"

#include <Inventor/system/gl.h>
#include <Inventor/SbBox.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/misc/SoState.h>
#include "iostream"

#define mMaxLevelOfDetail 4 

SbList<SbVec3f> SoBeachBall::res2coords_;
SbList<LODInfo> SoBeachBall::lodinfo_;
SbBool SoBeachBall::haserror_ = false;

SO_NODE_SOURCE(SoBeachBall);

// Clears the data structures
void LODInfo::clear()
{
    tricoordindices_[0].truncate( 0 );
    tricoordindices_[0] = 0;
    tricoordindices_[1].truncate( 0 );
    tricoordindices_[1] = 0;
    normals_[0].truncate( 0 );
    normals_[0] = 0;
    normals_[1].truncate( 0 );
    normals_[1] = 0;
}


void SoBeachBall::initClass()
{
    // Initialize type id variables.
    SO_NODE_INIT_CLASS(SoBeachBall, SoShape, "Shape");
    initTriangles( mMaxLevelOfDetail );
    if ( haserror_ )
	clearData();

    //printDebugInfo();
}


SoBeachBall::SoBeachBall() : currlod_(0)
{
    SO_NODE_CONSTRUCTOR(SoBeachBall);
    SO_NODE_ADD_FIELD(materialindex, (-1));
}


SoBeachBall::~SoBeachBall()
{
}


// This method creates and initializes the triangles for all levels of detail. 
void SoBeachBall::initTriangles( int numlevels )
{
    initVertices( numlevels );
    calculateNormals( numlevels );
}


// This method fills up the vertices, edges and faces data structures.
void SoBeachBall::initVertices( int numlevels )
{
    // Triangles approximating a sphere need to be generated for all levels
    // of detail and stored in the data structures for future use.
    // Lowest level of detail (level 0) is an octahedron (8 triangles).
    // Higher levels of detail possess 4 times more triangles than the 
    // previous level.
    const int res0numedges = 12, res0numvertices =  6;

    // The edges must be stored temporarily in order to compute the vertices
    // of the next level of detail. They are stored as pairs of indices into 
    // res2coords, representing the 2 vertices that each edge joins. 
    typedef SbList<int> EdgeIndicesType;
    SbList<EdgeIndicesType> edgeindices;

    // Initialise the first level of detail (octahedron)
    // Vertices
    res2coords_.append( SbVec3f( 1.0, 0.0, 0.0 ) );
    res2coords_.append( SbVec3f( 0.0, 1.0, 0.0 ) );
    res2coords_.append( SbVec3f( 0.0, 0.0, 1.0 ) );
    res2coords_.append( SbVec3f( -1.0, 0.0, 0.0 ) );
    res2coords_.append( SbVec3f( 0.0, -1.0, 0.0 ) );
    res2coords_.append( SbVec3f( 0.0, 0.0, -1.0 ) );

    // Edges
    EdgeIndicesType res0edgeindices;

    res0edgeindices.append( 0 );	res0edgeindices.append( 1 );
    res0edgeindices.append( 0 );	res0edgeindices.append( 2 );
    res0edgeindices.append( 0 );	res0edgeindices.append( 4 );
    res0edgeindices.append( 0 );	res0edgeindices.append( 5 );
    res0edgeindices.append( 3 );	res0edgeindices.append( 1 );
    res0edgeindices.append( 3 );	res0edgeindices.append( 2 );
    res0edgeindices.append( 3 );	res0edgeindices.append( 4 );
    res0edgeindices.append( 3 );	res0edgeindices.append( 5 );
    res0edgeindices.append( 1 );	res0edgeindices.append( 2 );
    res0edgeindices.append( 2 );	res0edgeindices.append( 4 );
    res0edgeindices.append( 1 );	res0edgeindices.append( 5 );
    res0edgeindices.append( 5 );	res0edgeindices.append( 4 );

    edgeindices.append( res0edgeindices );

    // Faces 
    LODInfo level0info;

    // triangles corresponding to color1
    level0info.tricoordindices_[0].append( 1 );	
    level0info.tricoordindices_[0].append( 0 );
    level0info.tricoordindices_[0].append( 2 );   

    level0info.tricoordindices_[0].append( 0 );
    level0info.tricoordindices_[0].append( 2 );   
    level0info.tricoordindices_[0].append( 4 );

    level0info.tricoordindices_[0].append( 1 );
    level0info.tricoordindices_[0].append( 3 );
    level0info.tricoordindices_[0].append( 5 );

    level0info.tricoordindices_[0].append( 3 );
    level0info.tricoordindices_[0].append( 5 );  
    level0info.tricoordindices_[0].append( 4 );
   
    // triangles corresponding to color2
    level0info.tricoordindices_[1].append( 1 );	
    level0info.tricoordindices_[1].append( 2 );
    level0info.tricoordindices_[1].append( 3 );   

    level0info.tricoordindices_[1].append( 2 );
    level0info.tricoordindices_[1].append( 3 );   
    level0info.tricoordindices_[1].append( 4 );

    level0info.tricoordindices_[1].append( 1 );
    level0info.tricoordindices_[1].append( 5 );
    level0info.tricoordindices_[1].append( 0 );

    level0info.tricoordindices_[1].append( 5 );
    level0info.tricoordindices_[1].append( 0 );  
    level0info.tricoordindices_[1].append( 4 );

    lodinfo_.append( level0info );
    level0info.clear();

    int endindex = res0numvertices - 1;
    int numedges = res0numedges;
    
    for ( int ilvl = 1; ilvl < numlevels; ilvl++ )
    {
	EdgeIndicesType newresedgeindices;
	LODInfo newlevelinfo;

	// fill up data structures for all levels of detail > 0
	tessellate( ilvl, endindex, 
		edgeindices[ilvl-1], newresedgeindices,
		lodinfo_[ilvl-1].tricoordindices_[0], 
		lodinfo_[ilvl-1].tricoordindices_[1],
		newlevelinfo.tricoordindices_[0], 
		newlevelinfo.tricoordindices_[1] );
	edgeindices.append( newresedgeindices );
	lodinfo_.append( newlevelinfo );
	  // note: normals will be filled in by method calculateNormals

	// Number of vertices at every level (greater than 0) is
	// v(ilvl) = v(ilvl-1) + e(ilvl-1)
	endindex = endindex + numedges;
	numedges = 4 * numedges;

	newresedgeindices.truncate( 0 );
	newlevelinfo.clear();
    }
}


// This method performs the tessellation (creates the next level of detail).
void SoBeachBall::tessellate( int ilevel, int endindex,
			SbList<int>& prevlvledges, SbList<int>& currlvledges,
			SbList<int>& prevlvlfaces1, SbList<int>& prevlvlfaces2,
			SbList<int>& currlvlfaces1, SbList<int>& currlvlfaces2 )
{
    // new vertices = midpoints of edges of previous level
    int inewvertex = endindex + 1;
    for ( int iedge = 0; iedge < prevlvledges.getLength(); inewvertex++ )
    {
        float x, y, z;
        const int v1 = prevlvledges[iedge];
        const int v2 = prevlvledges[iedge+1];

        x = ( res2coords_[v1][0] 
            + res2coords_[v2][0] ) / 2.0;
        y = ( res2coords_[v1][1] 
            + res2coords_[v2][1] ) / 2.0;        
        z = ( res2coords_[v1][2]
            + res2coords_[v2][2] ) / 2.0;

        // project (x, y, z) onto the surface of the sphere
        SbVec3f res( x, y, z );
	res.normalize();

	// add a new vertex
	res2coords_.append( res );  // at index inewvertex
            
	// add 2 new edges
        currlvledges.append( v1 );
        currlvledges.append( inewvertex );
        currlvledges.append( inewvertex );
        currlvledges.append( v2 );

        // note: processFaces will add 3 new edges for every old face

        iedge += 2;
    }

    // call ProcessFaces passing (among others) start and end indices into 
    // res2coords_ of the newly-added vertices
    processFaces( prevlvledges, currlvledges, prevlvlfaces1, currlvlfaces1, 
	    endindex+1, inewvertex-1 );
    processFaces( prevlvledges, currlvledges, prevlvlfaces2, currlvlfaces2, 
	    endindex+1, inewvertex-1 );
}


// Adds new faces and remaining edges connecting new vertices.
void SoBeachBall::processFaces( SbList<int>& prevlvledges, 
	SbList<int>& currlvledges, SbList<int>& prevlvlfaces, 
	SbList<int>& currlvlfaces, int startnewv, int endnewv )
{
    const int numprevlvlfaces = prevlvlfaces.getLength() / 3;

    for ( int iface = 0; iface < numprevlvlfaces; iface++ )
    {
	const int v1 = prevlvlfaces[iface*3];
	const int v2 = prevlvlfaces[iface*3+1];
	const int v3 = prevlvlfaces[iface*3+2];

	// Find vertex indices corresponding to these 3 vertices
	int a = findNewVertex( prevlvledges, startnewv, endnewv, v1, v2 );
        int b = findNewVertex( prevlvledges, startnewv, endnewv, v2, v3 );
        int c = findNewVertex( prevlvledges, startnewv, endnewv, v1, v3 );
     
        if ( ( a == -1 ) || ( b == -1 ) || ( c == -1 ) )
        {
            haserror_ = true;
            continue;  // should not reach here
        }

	// Add 4 new faces
	addFace( currlvlfaces, a, b, c );
	addFace( currlvlfaces, a, b, v2 );
	addFace( currlvlfaces, b, c, v3 );
	addFace( currlvlfaces, a, c, v1 );

        // Add 3 new edges
        currlvledges.append( a );
        currlvledges.append( b );
        currlvledges.append( b );
        currlvledges.append( c );
        currlvledges.append( a );
        currlvledges.append( c );
    }
}


// Finds the index of the vertex that has been added in between vertices v1 
// and v2.
int SoBeachBall::findNewVertex( SbList<int>& edges, int startnewv,
	int endnewv, int v1, int v2 )
{
    int newvertindex = -1;
    int v1index = edges.find( v1 );
    while ( v1index != -1 )
    {      
        newvertindex = checkEdge( edges, v1, v2, v1index );
        if ( newvertindex != -1 )
          break;      // success!
        // find next occurrence of v1 in edges - workaround because of no 
        // findNext method in SbList
        v1index++;
        while ( ( v1index <= edges.getLength() ) 
            && ( edges[v1index] != v1 ) )
            v1index++;
        if ( v1index >= edges.getLength() )
           break;  
    }
    if ( ( newvertindex >=0 ) && ( newvertindex <= ( endnewv - startnewv ) ) )
	return startnewv + newvertindex;
    return -1;
}


// Checks the predecessor and successor of v1 and returns the index of the 
// corresponding new vertex (into newvertindices).
int SoBeachBall::checkEdge( SbList<int>& edges, int v1, int v2, int index )
{
    // check predecessor, if any
    if ( (index != 0 ) && ( edges[index-1] == v2 ) && ( (index%2) == 1 ) )
        // last clause checks if the correct pair of vertices corresponding to  
        // an edge is accessed
        return (index-1)/2;
    // check successor, if any
    else if ( (index != edges.getLength()-1 ) 
               && ( edges[index+1] == v2 ) && ( (index%2) == 0 ) )
        return index/2;
    else return -1;
}


// Adds a new face to the given list.
void SoBeachBall::addFace( SbList<int>& faces, int p, int q, int r )
{
    faces.append( p );
    faces.append( q );
    faces.append( r );
}


// Calculates the normals for all levels of detail.
void SoBeachBall::calculateNormals( int numlevels )
{
    for ( int ilvl = 0; ilvl < numlevels; ilvl++ )
    {
	calculateNormals( &(lodinfo_[ilvl].tricoordindices_[0]), 
		&(lodinfo_[ilvl].normals_[0]) );
	calculateNormals( &(lodinfo_[ilvl].tricoordindices_[1]), 
		&(lodinfo_[ilvl].normals_[1]) );
    }
}


// Calculates the normals for all triangles in the given level of detail.
void SoBeachBall::calculateNormals( SbList<int>* ptrilist, 
				    SbList<SbVec3f>* pnormalslist )
{
    for ( int itri = 0; itri < ptrilist->getLength(); )
    {
	int v1 = ptrilist->operator[] ( itri );
	int v2 = ptrilist->operator[] ( itri+1 );
	int v3 = ptrilist->operator[] ( itri+2 );
	SbVec3f vec1 = res2coords_[v2] - res2coords_[v1];
	SbVec3f vec2 = res2coords_[v3] - res2coords_[v1];

	SbVec3f normal;
	// insert a predetermined normal in case the vertices do not form
	// a triangle
	if ( ( vec1.dot( vec2 ) ) == ( vec1.length() * vec2.length() ) )
	{
	    // should not reach here
	    normal = SbVec3f( 1.0, 1.0, 1.0 );            
	}
	else
	{
	    normal = vec1.cross( vec2 );
            normal.normalize();

    	    // check if normal points inward - ideally must test with
	    // the centre of the triangle; v1 is an approximation as
	    // the triangles are not large
           if ( normal.dot( res2coords_[v1] ) < 0 )
	   {
	       // inward-pointing normal; must be inverted to make it point
	       // outward so that this face will be illuminated
	       normal.negate();

	       // reorder vertices - swap v2 and v3
	       ptrilist->operator[] ( itri+1 ) = v3;
	       ptrilist->operator[] ( itri+2 ) = v2;
	   }
	}
	pnormalslist->append( normal );
	itri += 3;
    }
}


// Clears the all data structures
void SoBeachBall::clearData()
{
    res2coords_.truncate( 0 );
    res2coords_ = 0;
    for ( int ilvl = 0; ilvl < mMaxLevelOfDetail; ilvl++ )
    {
	lodinfo_[ilvl].clear();
    }
}

// The rendering method of this shape.
void SoBeachBall::GLRender( SoGLRenderAction* action )
{
    if ( res2coords_ == 0 )
        return;   
    SoState *state = action->getState();

    if ( !shouldGLRender( action ) )
        return;

    SoMaterialBindingElement::Binding binding =
       	SoMaterialBindingElement::get( state );
    SbBool materialperpart =
     ( binding == SoMaterialBindingElement::PER_PART || 
       binding == SoMaterialBindingElement::PER_PART_INDEXED );
    if ( !materialperpart )
	return;

    SbList<int>* ptrilist1 = 0; 
    SbList<int>* ptrilist2 = 0;
    SbList<SbVec3f>* pnormalslist1 = 0;
    SbList<SbVec3f>* pnormalslist2 = 0;
    
    getTriangleInfo( state, &ptrilist1, &ptrilist2, &pnormalslist1, 
	    &pnormalslist2 );
    
    if ( !ptrilist1 || !ptrilist2 || !pnormalslist1 || !pnormalslist2 )
	return;

    SbBool sendnormals = 
        ( SoLightModelElement::get( state ) != 
        SoLightModelElement::BASE_COLOR );

    SoMaterialBundle mb( action );
    if ( binding == SoMaterialBindingElement::PER_PART )
	mb.sendFirst();
    else if ( ( binding == SoMaterialBindingElement::PER_PART_INDEXED )
	      && ( materialindex.getNum() > 0 ) )
	mb.send( materialindex[0], FALSE );
        // Note: mb.sendFirst is not called in this case!   

    beginSolidShape( action );
    renderTriangles( ptrilist1, pnormalslist1, sendnormals );
    
    if ( testNumColors( state ) )
    {
	if ( binding == SoMaterialBindingElement::PER_PART )
	    mb.send( 1, FALSE );
	else if ( ( binding == SoMaterialBindingElement::PER_PART_INDEXED )
		&& ( materialindex.getNum() > 1 ) )
	    mb.send( materialindex[1], FALSE );
        renderTriangles( ptrilist2, pnormalslist2, sendnormals );
    }
    endSolidShape( action );
}


// Finds the data structures for the desired level of detail.
void SoBeachBall::getTriangleInfo( SoState* state, SbList<int>** ptrilist1, 
	SbList<int>** ptrilist2, SbList<SbVec3f>** pnormalslist1,
	SbList<SbVec3f>** pnormalslist2 )
{
    this->computeResolution( state );

    if ( currlod_ == -1 )
	return;

    *ptrilist1 = &(lodinfo_[currlod_].tricoordindices_[0]);
    *ptrilist2 = &(lodinfo_[currlod_].tricoordindices_[1]);
    *pnormalslist1 = &(lodinfo_[currlod_].normals_[0]);
    *pnormalslist2 = &(lodinfo_[currlod_].normals_[1]);
}


// Renders the triangles of half the sphere (2 opposite stripes).
void SoBeachBall::renderTriangles( SbList<int>* ptrilist, 
	SbList<SbVec3f>* pnormalslist, SbBool sendnormals )
{
    glBegin(GL_TRIANGLES);

    for ( int i = 0; i < ptrilist->getLength(); )
    {
        if ( sendnormals )
            glNormal3fv( pnormalslist->operator[]( i/3 ).getValue() );
        glVertex3fv( res2coords_[ptrilist->operator[]( i++ )].getValue() );
        glVertex3fv( res2coords_[ptrilist->operator[]( i++ )].getValue() );
        glVertex3fv( res2coords_[ptrilist->operator[]( i++ )].getValue() );
    }
    glEnd();
}


// Generates triangles representing the sphere.
void SoBeachBall::generatePrimitives( SoAction* action )
{
    // Depending on the level of detail desired, triangles are generated to
    // approximate a sphere
    if ( res2coords_ == 0 )
        return;   
  
    SoState *state = action->getState();

    SoMaterialBindingElement::Binding binding =
        SoMaterialBindingElement::get( state );
    SbBool materialPerPart =
     ( binding == SoMaterialBindingElement::PER_PART || 
       binding == SoMaterialBindingElement::PER_PART_INDEXED );
    if ( !materialPerPart )
	return;

    SbList<int>* ptrilist1 = 0; 
    SbList<int>* ptrilist2 = 0;
    SbList<SbVec3f>* pnormalslist1 = 0;
    SbList<SbVec3f>* pnormalslist2 = 0;
    
    getTriangleInfo( state, &ptrilist1, &ptrilist2, &pnormalslist1, 
	    &pnormalslist2 );
        
    if ( !ptrilist1 || !ptrilist2 || !pnormalslist1 || !pnormalslist2 )
	return;

    SoPrimitiveVertex pv;

    // Macro to set the "point" variable to store the primitive vertex’s point.
    SbVec3f point;


    #define GEN_VERTEX( pv, x, y, z, normal )	\
    	point.setValue( x, y, z );		\
	pv.setPoint( point );			\
        pv.setNormal( normal );			\
        shapeVertex( &pv )			


    beginShape( action, TRIANGLES );
    
    // render first 2 stripes of same color
    for ( int i = 0; i < ptrilist1->getLength(); )
    {
        float x, y, z;
        SbVec3f normalvec = pnormalslist1->operator[]( i/3 ).getValue();
        
        res2coords_[ptrilist1->operator[]( i )].getValue( x, y, z );
        GEN_VERTEX( pv, x, y, z, normalvec );
        i++;
        res2coords_[ptrilist1->operator[]( i )].getValue( x, y, z );
        GEN_VERTEX( pv, x, y, z, normalvec );
        i++;
        res2coords_[ptrilist1->operator[]( i )].getValue( x, y, z );
        GEN_VERTEX( pv, x, y, z, normalvec );
        i++;
    }

    endShape();

    if ( testNumColors( state ) )
    {
	pv.setMaterialIndex( 1 );    
	beginShape( action, TRIANGLES );

	// render second 2 stripes of same color
	for ( int i = 0; i < ptrilist1->getLength(); )
	{
	    float x, y, z;
	    SbVec3f normalvec = pnormalslist2->operator[]( i/3 ).getValue();
        
	    res2coords_[ptrilist2->operator[]( i )].getValue( x, y, z );
	    GEN_VERTEX( pv, x, y, z, normalvec );
	    i++;
	    res2coords_[ptrilist2->operator[]( i )].getValue( x, y, z );
	    GEN_VERTEX( pv, x, y, z, normalvec );
	    i++;
	    res2coords_[ptrilist2->operator[]( i )].getValue( x, y, z );
            GEN_VERTEX( pv, x, y, z, normalvec );
            i++;
        }
        
	endShape();
    }
}


// Checks if at least 2 diffuse colors are specified in the material.
SbBool SoBeachBall::testNumColors( SoState* state )
{
    const SoElement* elem = state->getConstElement( 
	    SoLazyElement::getClassStackIndex() );
    if ( elem && ( ((SoLazyElement*) elem)->getNumDiffuse() >= 2 ) )
    {
	return true;
	// later: can check if the diffuse color is ignored and the value of
	// the override flag
    }
    else
	return false;
}


// Computes the bounding box and center of the beachball.
void SoBeachBall::computeBBox( SoAction*, SbBox3f& box, SbVec3f &center )
{
    if ( res2coords_ == 0 )
        return;   
 
    box.setBounds( SbVec3f( -1, -1, -1 ), SbVec3f( 1, 1, 1 ) );
    center.setValue( 0.0, 0.0, 0.0 );
}


// Computes the level of detail depending on the value of SoComplexity node
// and the screen space occupied. (Only screen space complexity is implemented.)
void SoBeachBall::computeResolution( SoState* state )
{
    SbBox3f bbox;
    SbVec3f dummy;

//    computeBBox( 0, bbox, dummy );
    bbox.setBounds( SbVec3f( -1, -1, -1 ), SbVec3f( 1, 1, 1 ) );
 
    const int32_t camerainfo = SoCameraInfoElement::get(state);
    if ( (camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE)) )
	return;

    int desiredres = 0;
    {
	SbVec2s screensize;
	SoShape::getScreenSize( state, bbox, screensize );
	const float complexity = 
	    SbClamp(SoComplexityElement::get(state), 0.0f, 1.0f);

	// maximum number of pixels per triangle
	const int numpixelspertriangle = 7; 
	// find the minimum number of triangles to be rendered
	const float wantednumtriangles = 
	    complexity*screensize[0]*screensize[1]/numpixelspertriangle;

	if (wantednumtriangles >= 0)
	{
	    // Number of triangles or faces in a level is
	    // f(l) = power(4, l) * 8 (assuming l runs from 0 to max-1)
	    desiredres = int (floor(log(float(wantednumtriangles/8))/log(4.)));

	    if ( desiredres >= mMaxLevelOfDetail )
		desiredres = mMaxLevelOfDetail - 1;
	    else if ( desiredres < 0 )
		desiredres = 0;
	}
    }
    this->currlod_ = (char) desiredres;
}


// Computes the bounding box and center of the beachball.
void SoBeachBall::rayPick( SoRayPickAction* action )
{
    if ( !shouldRayPick( action ) )
	return;
    action->setObjectSpace();
    const SbLine & line = action->getLine();
    SbSphere sphere( SbVec3f( 0.0f, 0.0f, 0.0f ), 1.0f );
    SbVec3f enter, exit;
    if ( sphere.intersect( line, enter, exit ) ) 
    {
	tryAddIntersection( action, enter );
	if ( exit != enter )
	   tryAddIntersection( action, exit );
    }
}

// Adds an intersection to the ray pick action.
void SoBeachBall::tryAddIntersection(SoRayPickAction* action, const SbVec3f& pt)
{
    if ( action->isBetweenPlanes( pt ) ) 
    {
	SoPickedPoint * pp = action->addIntersection( pt );
	if (pp)
	{
	    SbVec3f normal = pt;
	    normal.normalize();
	    pp->setObjectNormal( normal ); 
	}
    }
}


// Prints the coordinates of the triangles - debug info
void SoBeachBall::printDebugInfo()
{
    for ( int ilvl = 0; ilvl < mMaxLevelOfDetail; ilvl++ )
    {
	std::cout << "Level " << ilvl << ": faces in stripes 1 & 3: " 
	    << std::endl;
	printTriangles(lodinfo_[ilvl].tricoordindices_[0], 
		lodinfo_[ilvl].normals_[0]);
	std::cout << "Level " << ilvl << ": faces in stripes 2 & 4: " 
	    << std::endl;
	printTriangles(lodinfo_[ilvl].tricoordindices_[1], 
		lodinfo_[ilvl].normals_[1]);
    }
}


// Prints the coordinates of the vertices and normals of the triangles 
// - debug info
void SoBeachBall::printTriangles( SbList<int>& trilist, 
				  SbList<SbVec3f>& normalslist )
{
    for( int i = 0; i < trilist.getLength();)
    {
        std::cout << i/3 << "\t";
        std::cout << "("
                  << res2coords_[trilist[i]][0]
                  << ", "
                  << res2coords_[trilist[i]][1]
                  << ", "
                  << res2coords_[trilist[i]][2]
                  << ")\t";
        i++;
        std::cout << "("
                  << res2coords_[trilist[i]][0]
                  << ", "
                  << res2coords_[trilist[i]][1]
                  << ", "
                  << res2coords_[trilist[i]][2]
                  << ")\t";
        i++;
        std::cout << "("
                  << res2coords_[trilist[i]][0]
                  << ", "
                  << res2coords_[trilist[i]][1]
                  << ", "
                  << res2coords_[trilist[i]][2]
                  << ")\t";
	// the normal
	std::cout << "("
	          << normalslist[i/3][0]
		  << ", "
		  << normalslist[i/3][1]
		  << ", "
		  << normalslist[i/3][2]
		  << ")"
                  << std::endl;
       i++;
    }
}





