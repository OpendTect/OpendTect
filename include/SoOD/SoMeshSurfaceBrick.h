#ifndef SoMeshSurfaceBrick_h
#define SoMeshSurfaceBrick_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoMeshSurfaceBrick.h,v 1.3 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoIndexedTriangleStripSet.h"
#include "Inventor/fields/SoMFInt32.h"
#include "Inventor/fields/SoMFVec3f.h"
#include "Inventor/fields/SoSFShort.h"
#include "Inventor/lists/SbList.h"


class SbMutex;
class SbVec3f;
class SbVec2s;
class SoAction;

/*!
The class assumes that the coords (both given directly to it and on the state)
are organized in a grid where index=row*((sideSize+1)*spacing)+1)+col*spacing;

*/


class SoMeshSurfaceBrick : public SoIndexedTriangleStripSet
{
    typedef SoIndexedTriangleStripSet	inherited;
    SO_NODE_HEADER(SoMeshSurfaceBrick);
public:
    			SoMeshSurfaceBrick();
    static void		initClass(void);

    SoMFVec3f		normals;
    SoSFShort		sideSize;
    			/*!< Number of cells. */
    SoSFShort		spacing;

    void		setCoordPtr(const SbVec3f*);

    void		build(bool lock);

    void		invalidate();
    void		doUpdate();
    int			getValidState() const;
    			/*<\retval 0 - Everything is OK
			   \retval 1 - needs update, but is displayable
			   \retval 2 - is not yet built - nothing to display
			*/

    inline short	getNormalIndex(int relrow, int relcol) const;
    void		invalidateNormal(int);
    SbBool		getNormal(int, SbVec3f& );
    
private:
    			~SoMeshSurfaceBrick();

    const SbVec3f*	coords;
    int			validstate;
    SbMutex*		buildmutex;

    void		GLRender(SoGLRenderAction*);


    void		startNewStrip(int,int,int&,int&,int&,int&,int&,bool&);
    void		expandStrip(int,int,int&,int&,int&,
	    			    int&,int&,bool&);
    void		createTriangle(int&,int,int,int,int,
	    				int=-2,int=-1,int=-2,int=-1);
    int			getValidIndex(int) const;
    bool		getBestDiagonal(int,int,int,int,bool) const;
    SbBool		computeNormal(int);

    SbList<short>	invalidNormals;

    inline SbBool	isUndefined(float) const;
    inline SbBool	isUndefined(const SbVec3f&) const;

    inline short	getCoordIndex(int relrow, int relcol) const;
    inline short	getCoordsPerRow() const;

    inline short	getNormalsPerRow() const;

};


inline SbBool SoMeshSurfaceBrick::isUndefined(float val) const
{ return ( (val>9.99999e29) && (val<1.00001e30) ); }


inline SbBool SoMeshSurfaceBrick::isUndefined(const SbVec3f& vec) const
{ return isUndefined(vec[0]) || isUndefined(vec[1]) || isUndefined(vec[2]); }


inline short SoMeshSurfaceBrick::getNormalIndex(int relrow, int relcol) const
{
    const int spacingcache = spacing.getValue();
    relrow /= spacingcache;
    relcol /= spacingcache;
    return relrow*(sideSize.getValue()+2)+relcol;
}


inline short SoMeshSurfaceBrick::getNormalsPerRow() const
{
    return sideSize.getValue()+2;
}


inline short SoMeshSurfaceBrick::getCoordsPerRow() const
{
    return (sideSize.getValue()+1)*spacing.getValue()+1;
}


inline short SoMeshSurfaceBrick::getCoordIndex(int relrow, int relcol) const
{
    const short spacingcache = spacing.getValue();
    return spacing.getValue()*(relrow*getCoordsPerRow() + relcol);
}


#endif
