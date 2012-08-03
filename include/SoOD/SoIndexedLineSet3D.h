#ifndef SoIndexedLineSet3D_h
#define SoIndexedLineSet3D_h

#include "soodmod.h"
#include <Inventor/nodes/SoIndexedShape.h>

#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/elements/SoGLDisplayList.h>

#include "soodbasic.h"

//#define USE_DISPLAYLIST_LINESET

class SbBox3f;
class SbVec3f;

mClass(SoOD) SoIndexedLineSet3D : public SoIndexedShape
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

protected:
    			~SoIndexedLineSet3D();
    void		generatePrimitives(SoAction*);
    void		GLRender(SoGLRenderAction*);
    void		rayPick (SoRayPickAction *action);

private:
    friend		class SoLineSet3D;

    mClass(SoOD) LineSet3DData
    {
    public:
				LineSet3DData();
				~LineSet3DData();
	void			generateCoordinates(SoNode*,
					float radius, bool screensize, float
					maxradius, const int* ci, int nci,
					SoState*);
	void			glRender(const int32_t* materialindexes,
					 SoGLRenderAction*);
	bool			areCoordsValid(SoState*,SoNode*,bool) const;
    protected:
	bool			getEdgeStartCoords( const SbVec3f& edgecoord,
						    const SbVec3f& coord2,
						    SbVec3f* res, SoState* );
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

#ifdef USE_DISPLAYLIST_LINESET
	SoGLDisplayList*		displaylist_;
#endif
    };

    LineSet3DData			data_;
};


#endif

