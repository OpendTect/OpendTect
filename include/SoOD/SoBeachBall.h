#ifndef SoBeachBall_h
#define SoBeachBall_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Karthika
 Date:		July 2009
 RCS:		
________________________________________________________________________

-*/

#include <Inventor/SbColor.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/nodes/SoShape.h>
#include "soodbasic.h"

// SoBeachBall class for drawing a sphere with 4 stripes colored according to 
// 2 materials. Varying levels of detail of the beachball are supported.
class SoBeachBall : public SoShape 
{
    typedef SoShape inherited;
    SO_NODE_HEADER(SoBeachBall);

public:
    // field for specifying material indices for indexed binding
    SoMFInt32			materialIndex;

    				SoBeachBall();
    static void			initClass();

protected:
    // info about the vertices of the triangles for every level of detail.
    // X, Y, Z coordinates of the vertices of the highest level of detail.  
    // Vertices of lower levels of detail are subsets of this array.
    static SbList<SbVec3f>	res2Coords_;
    
    // info  about triangles in each level - 2 lists of indices per level of
    // detail into res2coords_, one list for every 2 stripes of same color.
    // Indices are stored in the form of triplets for every triangle.
    static SbList<int>		res0TriCoordIndices_[2];
    static SbList<int>		res1TriCoordIndices_[2];
    static SbList<int>		res2TriCoordIndices_[2];

    // normals for all the triangles
    static SbList<SbVec3f>	res0Normals_[2];
    static SbList<SbVec3f>	res1Normals_[2];
    static SbList<SbVec3f>	res2Normals_[2];

    static SbBool		hasError_;

    // methods
    virtual void 		GLRender(SoGLRenderAction *action);
    virtual void 		generatePrimitives(SoAction *action);
    virtual void 		computeBBox(SoAction *action, SbBox3f &box, 
	    				    SbVec3f &center);
    virtual void                rayPick(SoRayPickAction*);
         
    virtual			~SoBeachBall();
    
    static void			initTriangles();
    static void			initVertices();
    static void 		tessellate(int iLevel, int endIndex, 
	    				   SbList<int>& prevLvlEdges,
					   SbList<int>& currLvlEdges,
					   SbList<int>& prevLvlFaces1,
					   SbList<int>& prevLvlFaces2,
					   SbList<int>& currLvlFaces1,
					   SbList<int>& currLvlFaces2);
    static void 		processFaces(SbList<int>& prevLvlEdges,
	    				     SbList<int>& currLvlEdges,
					     SbList<int>& prevLvlFaces,
					     SbList<int>& currLvlFaces,
					     SbList<int>& newVertIndices);
    static int			findNewVertex(SbList<int>& edges, 
	    				      SbList<int>& newVertices, 
					      int v1, int v2);
    static int			checkEdge(SbList<int>& edges, int v1, int v2, 
	    				  int index);
    static void			addFace(SbList<int>& faces, int p, int q, 
	    				int r);
    static void			calculateNormals();
    static void			calculateNormals(SbList<int>* pTriList,
	    					 SbList<SbVec3f>* pNormalsList);
    static void			clearData();
    static void			getTriangleInfo(SbList<int>** pTriList1, 
	    					SbList<int>** pTriList2,
						SbList<SbVec3f>** pNormalsList1,
						SbList<SbVec3f>** pNormalsList2
						);
    static void 		renderTriangles(SbList<int>* pTriList,
	   					SbList<SbVec3f>* pNormalsList,
						SbBool sendNormals);
    SbBool			testNumColors( SoState *state );
    static void			tryAddIntersection(SoRayPickAction* action,
	    					   const SbVec3f& pt);
    static void			printDebugInfo();
    static void			printTriangles(SbList<int>& triList,
	    		  		       SbList<SbVec3f>& normalsList);
};

#endif
