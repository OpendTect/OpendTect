#ifndef SoIndexedLineSet3D_h
#define SoIndexedLineSet3D_h

#include <Inventor/nodes/SoIndexedShape.h>

#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFBool.h>

#include "soodbasic.h"

class SbBox3f;
class SbVec3f;

mClass SoIndexedLineSet3D : public SoIndexedShape
{
    SO_NODE_HEADER(SoIndexedLineSet3D);
public:

    static void		initClass();
    			SoIndexedLineSet3D();

    SoSFFloat		radius;
    SoSFBool		screenSize;
    			//!<Specifies wether radius is on screen or in display
			//!<Coordinates.
    SoSFFloat		maxRadius;
    			//!<Specifies maximum world radius if screenSize
			//!<is enabled. Less than zero means there is no
			//!<maximum
			//!<Coordinates.
    SoSFBool		rightHandSystem;
    			//!<Specifies wether the coordinate system is
			//!<righthanded or not.

protected:
    void		generatePrimitives(SoAction*);
    void		GLRender(SoGLRenderAction*);
    void		computeBBox( SoAction*, SbBox3f&, SbVec3f& );

private:
    bool	getEdgeStartCoords( const SbVec3f& edgecoord,
	    			    const SbVec3f& coord2,
				    SbVec3f* res, SoState* );
    void	generateTriangles( SoAction*, bool render );
    void	generateCoordinates( SoAction*, int startindex,
	    			     SbList<SbVec3f>&, SbList<SbVec3f>&,
	    			     SbList<SbVec3f>&, SbList<SbVec3f>&,
				     SbList<SbBool>&,
				     SbList<SbVec3f>& endnormals, int&,
				     SbBool world);


    static bool	didwarn;
};


#endif
