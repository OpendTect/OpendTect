#ifndef SoIndexedPolyLine3D_h
#define SoIndexedPolyLine3D_h

#include <Inventor/nodes/SoIndexedShape.h>

#include <Inventor/fields/SoSFFloat.h>

class SbBox3f;
class SbVec3f;

class SoIndexedPolyLine3D : public SoIndexedShape
{
    SO_NODE_HEADER(SoIndexedPolyLine3D);
public:

    static void		initClass();
    			SoIndexedPolyLine3D();

    SoSFFloat		radius;

protected:
    void		generatePrimitives(SoAction*);
    void		GLRender(SoGLRenderAction*);
    void		computeBBox( SoAction*, SbBox3f&, SbVec3f& );

private:
    bool	getEdgeStartCoords( const SbVec3f& edgecoord,
	    			    const SbVec3f& coord2,
				    SbVec3f* res);
    void	generateTriangles( SoAction*, bool render );
    void	generateCoordinates( SoAction*, int startindex,
	    			     SbVec3f*, SbVec3f*,
	    			     SbVec3f*, SbVec3f*, SbBool*,
				     SbVec3f* endnormals, int&, SbBool world);


    static bool	didwarn;
};


#endif
