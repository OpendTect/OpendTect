/*
___________________________________________________________________
/
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Karthika
 * DATE     : July 2009
___________________________________________________________________

-*/

#include "SoBeachBall.h"

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
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/misc/SoState.h>
#include "iostream"

#define mMaxLevelOfDetail 3

SbList<SbVec3f> SoBeachBall::res2Coords_;
SbList<int> SoBeachBall::res0TriCoordIndices_[2];
SbList<int> SoBeachBall::res1TriCoordIndices_[2];
SbList<int> SoBeachBall::res2TriCoordIndices_[2];
SbList<SbVec3f> SoBeachBall::res0Normals_[2];
SbList<SbVec3f> SoBeachBall::res1Normals_[2];
SbList<SbVec3f> SoBeachBall::res2Normals_[2];
SbBool SoBeachBall::hasError_ = false;

SO_NODE_SOURCE(SoBeachBall);

void SoBeachBall::initClass()
{
    // Initialize type id variables.
    SO_NODE_INIT_CLASS(SoBeachBall, SoShape, "Shape");
    initTriangles();
    if ( hasError_ )
	clearData();

    //printDebugInfo();
}


SoBeachBall::SoBeachBall() 
{
    SO_NODE_CONSTRUCTOR(SoBeachBall);
    SO_NODE_ADD_FIELD(materialIndex, (-1));
}


SoBeachBall::~SoBeachBall()
{
}


// This method creates and initializes the triangles for all levels of 
// detail.
void SoBeachBall::initTriangles()
{
    initVertices();
    calculateNormals();
}


// This method fills up the vertices, edges and faces data structures.
void SoBeachBall::initVertices()
{
    // Triangles approximating a sphere need to be generated for all levels
    // of detail and stored in the data structures for future use.
    // Lowest level of detail (level 0) is an octahedron (8 triangles).
    // Higher levels of detail possess 4 times more triangles than the 
    // previous level.
    const int res0NumEdges = 12, res0NumVertices =  6, res0NumFaces = 8;

    // The edges must be stored temporarily in order to compute the vertices
    // of the next level of detail. They are stored as pairs of indices into 
    // res2TriCoords, representing the 2 vertices that each edge joins. 
    static SbList<int> res0EdgeIndices;
    static SbList<int> res1EdgeIndices;
    static SbList<int> res2EdgeIndices;

    // Number of vertices at every level (greater than 0) is
    // v(ilvl) = v(ilvl-1) + e(ilvl-1)
    int res0EndIndex = res0NumVertices - 1;
    int res1EndIndex = res0EndIndex + res0NumEdges;


    // Initialise the first level of detail (octahedron)
    // Vertices
    res2Coords_.append( SbVec3f( 1.0, 0.0, 0.0 ) );
    res2Coords_.append( SbVec3f( 0.0, 1.0, 0.0 ) );
    res2Coords_.append( SbVec3f( 0.0, 0.0, 1.0 ) );
    res2Coords_.append( SbVec3f( -1.0, 0.0, 0.0 ) );
    res2Coords_.append( SbVec3f( 0.0, -1.0, 0.0 ) );
    res2Coords_.append( SbVec3f( 0.0, 0.0, -1.0 ) );

    // Edges
    res0EdgeIndices.append( 0 );	res0EdgeIndices.append( 1 );
    res0EdgeIndices.append( 0 );	res0EdgeIndices.append( 2 );
    res0EdgeIndices.append( 0 );	res0EdgeIndices.append( 4 );
    res0EdgeIndices.append( 0 );	res0EdgeIndices.append( 5 );
    res0EdgeIndices.append( 3 );	res0EdgeIndices.append( 1 );
    res0EdgeIndices.append( 3 );	res0EdgeIndices.append( 2 );
    res0EdgeIndices.append( 3 );	res0EdgeIndices.append( 4 );
    res0EdgeIndices.append( 3 );	res0EdgeIndices.append( 5 );
    res0EdgeIndices.append( 1 );	res0EdgeIndices.append( 2 );
    res0EdgeIndices.append( 2 );	res0EdgeIndices.append( 4 );
    res0EdgeIndices.append( 1 );	res0EdgeIndices.append( 5 );
    res0EdgeIndices.append( 5 );	res0EdgeIndices.append( 4 );

    // Faces 
    // triangles corresponding to color1
    res0TriCoordIndices_[0].append( 1 );	
    res0TriCoordIndices_[0].append( 0 );
    res0TriCoordIndices_[0].append( 2 );   

    res0TriCoordIndices_[0].append( 0 );
    res0TriCoordIndices_[0].append( 2 );   
    res0TriCoordIndices_[0].append( 4 );

    res0TriCoordIndices_[0].append( 1 );
    res0TriCoordIndices_[0].append( 3 );
    res0TriCoordIndices_[0].append( 5 );

    res0TriCoordIndices_[0].append( 3 );
    res0TriCoordIndices_[0].append( 5 );  
    res0TriCoordIndices_[0].append( 4 );
    
    // triangles corresponding to color2
    res0TriCoordIndices_[1].append( 1 );	
    res0TriCoordIndices_[1].append( 2 );
    res0TriCoordIndices_[1].append( 3 );   

    res0TriCoordIndices_[1].append( 2 );
    res0TriCoordIndices_[1].append( 3 );   
    res0TriCoordIndices_[1].append( 4 );

    res0TriCoordIndices_[1].append( 1 );
    res0TriCoordIndices_[1].append( 5 );
    res0TriCoordIndices_[1].append( 0 );

    res0TriCoordIndices_[1].append( 5 );
    res0TriCoordIndices_[1].append( 0 );  
    res0TriCoordIndices_[1].append( 4 );

    // fill up data structures for all levels of detail > 0
    tessellate( 1, res0EndIndex, res0EdgeIndices, res1EdgeIndices,
	   res0TriCoordIndices_[0], res0TriCoordIndices_[1],
	   res1TriCoordIndices_[0], res1TriCoordIndices_[1] );
    tessellate( 2, res1EndIndex, res1EdgeIndices, res2EdgeIndices,
	   res1TriCoordIndices_[0], res1TriCoordIndices_[1],
	   res2TriCoordIndices_[0], res2TriCoordIndices_[1] );
}


// This method performs the tessellation (creates the next level of detail).
void SoBeachBall::tessellate( int iLevel, int endIndex,
			SbList<int>& prevLvlEdges, SbList<int>& currLvlEdges,
			SbList<int>& prevLvlFaces1, SbList<int>& prevLvlFaces2,
			SbList<int>& currLvlFaces1, SbList<int>& currLvlFaces2 )
{
    SbList<int> newVertIndices;

    // new vertices = midpoints of edges of previous level
    int iNewVertex = endIndex + 1;
    for ( int iEdge = 0; iEdge < prevLvlEdges.getLength(); iNewVertex++ )
    {
        float x, y, z;
        const int v1 = prevLvlEdges[iEdge];
        const int v2 = prevLvlEdges[iEdge+1];

        x = ( res2Coords_[v1][0] 
            + res2Coords_[v2][0] ) / 2.0;
        y = ( res2Coords_[v1][1] 
            + res2Coords_[v2][1] ) / 2.0;        
        z = ( res2Coords_[v1][2]
            + res2Coords_[v2][2] ) / 2.0;

        // project (x, y, z) onto the surface of the sphere
        SbVec3f res( x, y, z );
	res.normalize();

	// add a new vertex
	res2Coords_.append( res );  // at index iNewVertex
            
	// add 2 new edges
        currLvlEdges.append( v1 );
        currLvlEdges.append( iNewVertex );
        currLvlEdges.append( iNewVertex );
        currLvlEdges.append( v2 );

        // note: processFaces will add 3 new edges for every old face

	newVertIndices.append( iNewVertex );
        iEdge += 2;
    }

    processFaces( prevLvlEdges, currLvlEdges, 
        prevLvlFaces1, currLvlFaces1, newVertIndices );
    processFaces( prevLvlEdges, currLvlEdges, 
        prevLvlFaces2, currLvlFaces2, newVertIndices );
}


// Adds new faces and remaining edges connecting new vertices.
void SoBeachBall::processFaces( SbList<int>& prevLvlEdges, 
	SbList<int>& currLvlEdges, SbList<int>& prevLvlFaces, 
	SbList<int>& currLvlFaces, SbList<int>& newVertices )
{
    const int numPrevLvlFaces = prevLvlFaces.getLength() / 3;

    for ( int iFace = 0; iFace < numPrevLvlFaces; iFace++ )
    {
	const int v1 = prevLvlFaces[iFace*3];
	const int v2 = prevLvlFaces[iFace*3+1];
	const int v3 = prevLvlFaces[iFace*3+2];

	// Find vertex indices corresponding to these 3 vertices
	int a = findNewVertex( prevLvlEdges, newVertices, v1, v2 );
        int b = findNewVertex( prevLvlEdges, newVertices, v2, v3 );
        int c = findNewVertex( prevLvlEdges, newVertices, v1, v3 );
     
        if ( ( a == -1 ) || ( b == -1 ) || ( c == -1 ) )
        {
            hasError_ = true;
            continue;  // should not reach here
        }

	// Add 4 new faces
	addFace( currLvlFaces, a, b, c );
	addFace( currLvlFaces, a, b, v2 );
	addFace( currLvlFaces, b, c, v3 );
	addFace( currLvlFaces, a, c, v1 );

        // Add 3 new edges
        currLvlEdges.append( a );
        currLvlEdges.append( b );
        currLvlEdges.append( b );
        currLvlEdges.append( c );
        currLvlEdges.append( a );
        currLvlEdges.append( c );
    }
}


// Finds the index of the vertex that has been added in between vertices v1 
// and v2.
int SoBeachBall::findNewVertex( SbList<int>& edges, SbList<int>& newVertIndices,
				int v1, int v2 )
{
    int newVertIndex = -1;
    int v1Index = edges.find( v1 );
    while ( v1Index != -1 )
    {      
        newVertIndex = checkEdge( edges, v1, v2, v1Index );
        if ( newVertIndex != -1 )
          break;      // success!
        // find next occurrence of v1 in edges - workaround because of no 
        // findNext method in SbList
        v1Index++;
        while ( ( v1Index <= edges.getLength() ) 
            && ( edges[v1Index] != v1 ) )
            v1Index++;
        if ( v1Index >= edges.getLength() )
           break;  
    }
    if ( ( newVertIndex >=0 ) 
          && ( newVertIndex <= newVertIndices.getLength () ) )
        return newVertIndices[newVertIndex];
    return -1;
}


// Checks the predecessor and successor of v1 and returns the index of the 
// corresponding new vertex (into newVertIndices).
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
void SoBeachBall::calculateNormals()
{
    calculateNormals( &(res0TriCoordIndices_[0]), &(res0Normals_[0]) );
    calculateNormals( &(res0TriCoordIndices_[1]), &(res0Normals_[1]) );
    calculateNormals( &(res1TriCoordIndices_[0]), &(res1Normals_[0]) );
    calculateNormals( &(res1TriCoordIndices_[1]), &(res1Normals_[1]) );
    calculateNormals( &(res2TriCoordIndices_[0]), &(res2Normals_[0]) );
    calculateNormals( &(res2TriCoordIndices_[1]), &(res2Normals_[1]) );
}


// Calculates the normals for all triangles in the given level of detail.
void SoBeachBall::calculateNormals( SbList<int>* pTriList, 
				    SbList<SbVec3f>* pNormalsList )
{
    for ( int iTri = 0; iTri < pTriList->getLength(); )
    {
	int v1 = pTriList->operator[] ( iTri );
	int v2 = pTriList->operator[] ( iTri+1 );
	int v3 = pTriList->operator[] ( iTri+2 );
	SbVec3f vec1 = res2Coords_[v2] - res2Coords_[v1];
	SbVec3f vec2 = res2Coords_[v3] - res2Coords_[v1];

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
           if ( normal.dot( res2Coords_[v1] ) < 0 )
	   {
	       // inward-pointing normal; must be inverted to make it point
	       // outward so that this face will be illuminated
	       normal.negate();

	       // reorder vertices - swap v2 and v3
	       pTriList->operator[] ( iTri+1 ) = v3;
	       pTriList->operator[] ( iTri+2 ) = v2;
	   }
	}
	pNormalsList->append( normal );
	iTri += 3;
    }
}


// Clears the all data structures
void SoBeachBall::clearData()
{
    res2Coords_.truncate( 0 );
    res2Coords_ = 0;
    res0TriCoordIndices_[0].truncate( 0 );
    res0TriCoordIndices_[1].truncate( 0 );
    res1TriCoordIndices_[0].truncate( 0 );
    res1TriCoordIndices_[1].truncate( 0 );
    res2TriCoordIndices_[0].truncate( 0 );
    res2TriCoordIndices_[1].truncate( 0 );
    res0TriCoordIndices_[0] = 0;	
    res0TriCoordIndices_[1] = 0;	
    res1TriCoordIndices_[0] = 0;	
    res1TriCoordIndices_[1] = 0;	
    res2TriCoordIndices_[0] = 0;	
    res2TriCoordIndices_[1] = 0;
    res0Normals_[0].truncate( 0 );
    res0Normals_[1].truncate( 0 );
    res1Normals_[0].truncate( 0 );
    res1Normals_[1].truncate( 0 );
    res2Normals_[0].truncate( 0 );
    res2Normals_[1].truncate( 0 );
    res0Normals_[0] = 0;
    res0Normals_[1] = 0;
    res1Normals_[0] = 0;
    res1Normals_[1] = 0;
    res2Normals_[0] = 0;
    res2Normals_[1] = 0;
}

// The rendering method of this shape.
void SoBeachBall::GLRender( SoGLRenderAction *action )
{
    if ( res2Coords_ == 0 )
        return;   
    SoState *state = action->getState();

    if ( !shouldGLRender( action ) )
        return;

    SoMaterialBindingElement::Binding binding =
       	SoMaterialBindingElement::get( state );
    SbBool materialPerPart =
     ( binding == SoMaterialBindingElement::PER_PART || 
       binding == SoMaterialBindingElement::PER_PART_INDEXED );
    if ( !materialPerPart )
	return;

    SbBool sendNormals = 
        ( SoLightModelElement::get( state ) != 
        SoLightModelElement::BASE_COLOR );

    SoMaterialBundle mb( action );
    if ( binding == SoMaterialBindingElement::PER_PART )
	mb.sendFirst();
    else if ( ( binding == SoMaterialBindingElement::PER_PART_INDEXED )
	      && ( materialIndex.getNum() > 0 ) )
	mb.send( materialIndex[0], FALSE );
        // Note: mb.sendFirst is not called in this case!   

    SbList<int>* pTriList1 = 0; 
    SbList<int>* pTriList2 = 0;
    SbList<SbVec3f>* pNormalsList1 = 0;
    SbList<SbVec3f>* pNormalsList2 = 0;
    
    getTriangleInfo( &pTriList1, &pTriList2, &pNormalsList1, &pNormalsList2 );
    beginSolidShape( action );
    renderTriangles( pTriList1, pNormalsList1, sendNormals );
    
    if ( testNumColors( state ) )
    {
	if ( binding == SoMaterialBindingElement::PER_PART )
	    mb.send( 1, FALSE );
	else if ( ( binding == SoMaterialBindingElement::PER_PART_INDEXED )
		&& ( materialIndex.getNum() > 1 ) )
	    mb.send( materialIndex[1], FALSE );
        renderTriangles( pTriList2, pNormalsList2, sendNormals );
    }
    endSolidShape( action );
}


// Finds the data structures for the desired level of detail.
void SoBeachBall::getTriangleInfo( SbList<int>** pTriList1, 
	SbList<int>** pTriList2, SbList<SbVec3f>** pNormalsList1,
	SbList<SbVec3f>** pNormalsList2 )
{
    // For now, we use an internal float to specify the level of detail
    // This must later be retrieved from the SoDetail class
    // 0-0.3 - level 0; 0.3-0.7 - level 1; 0.7-1.0 - level 2
    const float levelOfDetail = 0.9;
    const int level = (int) floor ( levelOfDetail * mMaxLevelOfDetail );

    if ( level == 0) 
    { 
        *pTriList1 = &(res0TriCoordIndices_[0]);
        *pTriList2 = &(res0TriCoordIndices_[1]);
        *pNormalsList1 = &(res0Normals_[0]);
        *pNormalsList2 = &(res0Normals_[1]);
    }
    else if ( level == 1)
    { 
        *pTriList1 = &(res1TriCoordIndices_[0]);
        *pTriList2 = &(res1TriCoordIndices_[1]);
        *pNormalsList1 = &(res1Normals_[0]);
        *pNormalsList2 = &(res1Normals_[1]);
    }
    else
    { 
        *pTriList1 = &(res2TriCoordIndices_[0]);
        *pTriList2 = &(res2TriCoordIndices_[1]);
        *pNormalsList1 = &(res2Normals_[0]);
        *pNormalsList2 = &(res2Normals_[1]);
    }
}


// Renders the triangles of half the sphere (2 opposite stripes).
void SoBeachBall::renderTriangles( SbList<int>* pTriList, 
	SbList<SbVec3f>* pNormalsList, SbBool sendNormals )
{
    glBegin(GL_TRIANGLES);

    for ( int i = 0; i < pTriList->getLength(); )
    {
        if ( sendNormals )
            glNormal3fv( pNormalsList->operator[]( i/3 ).getValue() );
        glVertex3fv( res2Coords_[pTriList->operator[]( i++ )].getValue() );
        glVertex3fv( res2Coords_[pTriList->operator[]( i++ )].getValue() );
        glVertex3fv( res2Coords_[pTriList->operator[]( i++ )].getValue() );
    }
    glEnd();
}


// Generates triangles representing the sphere.
void SoBeachBall::generatePrimitives( SoAction *action )
{
    // Depending on the level of detail desired, triangles are generated to
    // approximate a sphere
    if ( res2Coords_ == 0 )
        return;   
  
    SoState *state = action->getState();

    SoMaterialBindingElement::Binding binding =
        SoMaterialBindingElement::get( state );
    SbBool materialPerPart =
     ( binding == SoMaterialBindingElement::PER_PART || 
       binding == SoMaterialBindingElement::PER_PART_INDEXED );
    if ( !materialPerPart )
	return;

    SbBool sendNormals = 
        ( SoLightModelElement::get( state ) != 
        SoLightModelElement::BASE_COLOR );

    SbList<int>* pTriList1 = 0; 
    SbList<int>* pTriList2 = 0;
    SbList<SbVec3f>* pNormalsList1 = 0;
    SbList<SbVec3f>* pNormalsList2 = 0;
    
    getTriangleInfo( &pTriList1, &pTriList2, &pNormalsList1, &pNormalsList2 );

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
    for ( int i = 0; i < pTriList1->getLength(); )
    {
        float x, y, z;
        SbVec3f normalvec = pNormalsList1->operator[]( i/3 ).getValue();
        
        res2Coords_[pTriList1->operator[]( i )].getValue( x, y, z );
        GEN_VERTEX( pv, x, y, z, normalvec );
        i++;
        res2Coords_[pTriList1->operator[]( i )].getValue( x, y, z );
        GEN_VERTEX( pv, x, y, z, normalvec );
        i++;
        res2Coords_[pTriList1->operator[]( i )].getValue( x, y, z );
        GEN_VERTEX( pv, x, y, z, normalvec );
        i++;
    }

    endShape();

    if ( testNumColors( state ) )
    {
	pv.setMaterialIndex( 1 );    
	beginShape( action, TRIANGLES );

	// render second 2 stripes of same color
	for ( int i = 0; i < pTriList1->getLength(); )
	{
	    float x, y, z;
	    SbVec3f normalvec = pNormalsList2->operator[]( i/3 ).getValue();
        
	    res2Coords_[pTriList2->operator[]( i )].getValue( x, y, z );
	    GEN_VERTEX( pv, x, y, z, normalvec );
	    i++;
	    res2Coords_[pTriList2->operator[]( i )].getValue( x, y, z );
	    GEN_VERTEX( pv, x, y, z, normalvec );
	    i++;
	    res2Coords_[pTriList2->operator[]( i )].getValue( x, y, z );
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
void SoBeachBall::computeBBox( SoAction *, SbBox3f &box, SbVec3f &center )
{
    if ( res2Coords_ == 0 )
        return;   
 
    box.setBounds( SbVec3f( -1, -1, -1 ), SbVec3f( 1, 1, 1 ) );
    center.setValue( 0.0, 0.0, 0.0 );
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
    std::cout << "Level 0: faces in stripes 1 & 3: " << std::endl;
    printTriangles(res0TriCoordIndices_[0], res0Normals_[0]);
    std::cout << "Level 0: faces in stripes 2 & 4: " << std::endl;
    printTriangles(res0TriCoordIndices_[1], res0Normals_[1]);
    std::cout << "Level 1: faces in stripes 1 & 3: " << std::endl;
    printTriangles(res1TriCoordIndices_[0], res1Normals_[0]);
    std::cout << "Level 1: faces in stripes 2 & 4: " << std::endl;
    printTriangles(res1TriCoordIndices_[1], res1Normals_[1]);
    std::cout << "Level 2: faces in stripes 1 & 3: " << std::endl;
    printTriangles(res2TriCoordIndices_[0], res2Normals_[0]);
    std::cout << "Level 2: faces in stripes 2 & 4: " << std::endl;
    printTriangles(res2TriCoordIndices_[1], res2Normals_[1]);
}


// Prints the coordinates of the vertices and normals of the triangles 
// - debug info
void SoBeachBall::printTriangles( SbList<int>& triList, 
				  SbList<SbVec3f>& normalsList )
{
    for( int i = 0; i < triList.getLength();)
    {
        std::cout << i/3 << "\t";
        std::cout << "("
                  << res2Coords_[triList[i]][0]
                  << ", "
                  << res2Coords_[triList[i]][1]
                  << ", "
                  << res2Coords_[triList[i]][2]
                  << ")\t";
        i++;
        std::cout << "("
                  << res2Coords_[triList[i]][0]
                  << ", "
                  << res2Coords_[triList[i]][1]
                  << ", "
                  << res2Coords_[triList[i]][2]
                  << ")\t";
        i++;
        std::cout << "("
                  << res2Coords_[triList[i]][0]
                  << ", "
                  << res2Coords_[triList[i]][1]
                  << ", "
                  << res2Coords_[triList[i]][2]
                  << ")\t";
	// the normal
	std::cout << "("
	          << normalsList[i/3][0]
		  << ", "
		  << normalsList[i/3][1]
		  << ", "
		  << normalsList[i/3][2]
		  << ")"
                  << std::endl;
       i++;
    }
}





