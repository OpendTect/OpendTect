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
    			~SoIndexedLineSet3D();
    void		generatePrimitives(SoAction*);
    void		GLRender(SoGLRenderAction*);
    void		rayPick (SoRayPickAction *action);

private:
    bool	getEdgeStartCoords( const SbVec3f& edgecoord,
	    			    const SbVec3f& coord2,
				    SbVec3f* res, SoState* );
    void	generateCoordinates( SoState* );
    bool	areCoordsValid( SoState* ) const;

    SbList<SbVec3f>		corner1_;
    SbList<SbVec3f>		corner2_;
    SbList<SbVec3f>		corner3_;
    SbList<SbVec3f>		corner4_;
    SbList<SbVec3f>		cornernormal1_;
    SbList<SbVec3f>		cornernormal2_;
    SbList<SbVec3f>		cornernormal3_;
    SbList<SbVec3f>		cornernormal4_;
    SbList<SbBool>		isreversed_;
    SbList<SbVec3f>		endnormals_;

    SbList<int>			sectionstarts_;

    int				nodeid_;
    SoElement*			modelmatchinfo_;
    SoElement*			coordmatchinfo_;
    SoElement*			vpmatchinfo_;
    SoElement*			vvmatchinfo_;
    bool			ismoving_;
};


#endif
