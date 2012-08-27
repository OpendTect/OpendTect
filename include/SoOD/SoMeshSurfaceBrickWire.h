#ifndef SoMeshSurfaceBrickWire_h
#define SoMeshSurfaceBrickWire_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoMeshSurfaceBrickWire.h,v 1.8 2012-08-27 13:16:48 cvskris Exp $
________________________________________________________________________


-*/

#include "soodmod.h"
#include "Inventor/nodes/SoIndexedLineSet.h"
#include "Inventor/fields/SoMFInt32.h"
#include "Inventor/fields/SoMFVec3f.h"
#include "Inventor/fields/SoSFShort.h"
#include "Inventor/lists/SbList.h"

#include "soodbasic.h"


class SbVec3f;
class SbVec2s;
class SoAction;

/*!
The class assumes that the coords (both given directly to it and on the state)
are organized in a grid where index=row*((sideSize+1)*spacing)+1)+col*spacing;

*/


mSoODClass SoMeshSurfaceBrickWire : public SoIndexedLineSet
{
    typedef SoIndexedLineSet	inherited;
    SO_NODE_HEADER(SoMeshSurfaceBrickWire);
public:
    			SoMeshSurfaceBrickWire();
    static void		initClass(void);

    SoSFShort		sideSize;
    			/*!< Number of cells. */
    SoSFShort		spacing;

    void		setCoordPtr(const SbVec3f*);

    void		build();

    void		invalidate();
    SbBool		isValid() const;

private:

    const SbVec3f*	coords;
    SbBool		invalidFlag;

    void		GLRender(SoGLRenderAction*);

    inline SbBool	isUndefined(float) const;
    inline SbBool	isUndefined(const SbVec3f&) const;

    inline short	getCoordIndex(int relrow, int relcol) const;
    inline short	getCoordsPerRow() const;
};


inline SbBool SoMeshSurfaceBrickWire::isUndefined(float val) const
{ return ( (val>9.99999e29) && (val<1.00001e30) ); }


inline SbBool SoMeshSurfaceBrickWire::isUndefined(const SbVec3f& vec) const
{ return isUndefined(vec[0]) || isUndefined(vec[1]) || isUndefined(vec[2]); }


inline short SoMeshSurfaceBrickWire::getCoordsPerRow() const
{
    return (sideSize.getValue()+1)*spacing.getValue()+1;
}


inline short SoMeshSurfaceBrickWire::getCoordIndex(int relrow, int relcol) const
{
    const short spacingcache = spacing.getValue();
    return spacing.getValue()*(relrow*getCoordsPerRow() + relcol);
}


#endif

