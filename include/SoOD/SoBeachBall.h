#ifndef SoBeachBall_h
#define SoBeachBall_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Karthika
 Date:		July 2009
 RCS:		$Id: SoBeachBall.h,v 1.8 2012-08-27 13:16:46 cvskris Exp $
________________________________________________________________________

-*/

#include "soodmod.h"
#include <Inventor/SbColor.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/nodes/SoShape.h>
#include "soodbasic.h"


// This class stores all info about a level of detail of the beachball, except
// the coordinate info (which is stored by res2coords_).
// 
mSoODClass LODInfo
{
public:
    
    // Triangle info - 2 lists of indices, one list for every 2 stripes of same
    // color. Indices are stored in the form of triplets for every triangle.
    SbList<int>			tricoordindices_[2];
    // normals for all the triangles
    SbList<SbVec3f>		normals_[2];

    				LODInfo() { };
				~LODInfo() { clear(); }
    void			clear();

};


// SoBeachBall class for drawing a sphere with 4 stripes colored according to 
// 2 materials. Varying levels of detail of the beachball are supported.
mSoODClass SoBeachBall : public SoShape 
{
    typedef SoShape inherited;
    SO_NODE_HEADER(SoBeachBall);

public:
    // field for specifying material indices for indexed binding
    SoMFInt32			materialindex;

    				SoBeachBall();
    static void			initClass();

protected:
    // info about the vertices of the triangles for every level of detail.
    // X, Y, Z coordinates of the vertices of the highest level of detail.  
    // vertices of lower levels of detail are subsets of this array.
    static SbList<SbVec3f>	res2coords_;
   
    // list of info about every level of detail supported
    static SbList<LODInfo>	lodinfo_;

    static SbBool		haserror_;

    char			currlod_;

    // methods
    virtual void 		GLRender(SoGLRenderAction* action);
    virtual void 		generatePrimitives(SoAction* action);
    virtual void 		computeBBox(SoAction* action, SbBox3f& box, 
	    				    SbVec3f& center);
    virtual void                rayPick(SoRayPickAction*);
         
    virtual			~SoBeachBall();
    
    static void			initTriangles(int numlevels);
    static void			initVertices(int numlevels);
    static void 		tessellate(int ilevel, int endindex, 
	    				   SbList<int>& prevlvledges,
					   SbList<int>& currlvledges,
					   SbList<int>& prevlvlfaces1,
					   SbList<int>& prevlvlfaces2,
					   SbList<int>& currlvlfaces1,
					   SbList<int>& currlvlfaces2);
    static void 		processFaces(SbList<int>& prevlvledges,
	    				     SbList<int>& currlvledges,
					     SbList<int>& prevlvlfaces,
					     SbList<int>& currlvlfaces,
					     int startnewv, int endnewv);
    static int			findNewVertex(SbList<int>& edges, 
	    					int startnewv, int endnewv, 
						int v1, int v2);
    static int			checkEdge(SbList<int>& edges, int v1, int v2, 
	    				  int index);
    static void			addFace(SbList<int>& faces, int p, int q,  
	    				int r);
    static void			calculateNormals( int numlevels );
    static void			calculateNormals(SbList<int>* ptrilist,
	    					 SbList<SbVec3f>* pnormalslist);
    static void			clearData();
    void			getTriangleInfo(SoState*state,
	    					SbList<int>** ptrilist1, 
	    					SbList<int>** ptrilist2,
						SbList<SbVec3f>** pnormalslist1,
						SbList<SbVec3f>** pnormalslist2
						);
    static void 		renderTriangles(SbList<int>* ptrilist,
	   					SbList<SbVec3f>* pnormalslist,
						SbBool sendnormals);
    static SbBool		testNumColors(SoState* state);
    void			computeResolution(SoState* state);
    static void			tryAddIntersection(SoRayPickAction* action,
	    					   const SbVec3f& pt);
    static void			printDebugInfo();
    static void			printTriangles(SbList<int>& triList,
	    		  		       SbList<SbVec3f>& normalsList);
};

#endif

