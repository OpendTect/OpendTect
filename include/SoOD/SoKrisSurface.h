#ifndef SoKrisSurface_h
#define SoKrisSurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoKrisSurface.h,v 1.1 2004-10-02 12:29:58 kristofer Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoShape.h"
#include "Inventor/fields/SoSFShort.h"
#include "Inventor/fields/SoMFInt32.h"
#include "Inventor/fields/SoSFInt32.h"
#include "Inventor/fields/SoSFBool.h"
#include "Inventor/fields/SoMFShort.h"
#include "Inventor/lists/SbList.h"
#include "Inventor/nodes/SoSubNode.h"

class SbVec3f;
class MeshSurfacePart;

class SoKrisSurface : public SoShape
{
    SO_NODE_HEADER(SoKrisSurface);
public:
    SoMFInt32		coordIndex;
    SoMFInt32		materialIndex;
    SoMFShort		meshStyle;
    			//0 - auto, 1 - diagonal between 0-3, 2 - diagonal between 1-2, 3-empty
    SoSFShort		nrColumns;
    SoSFShort		brickSize;
    SoSFShort		resolution;
    SoSFBool		wireframe;

    void		turnOnOwnValidation(bool yn);
    void		touch( int, int, bool undef );

    void		generatePrimitives(SoAction*) {}
    void		computeBBox(SoAction*, SbBox3f&, SbVec3f&);
    void		rayPick( SoRayPickAction*);
    void 		GLRender(SoGLRenderAction*);

    int			getCoordIndexIndex( int, int ) const;

    			SoKrisSurface();
    static void		initClass();

    int			nrRows() const;

protected:
    void		adjustNrOfParts();

    SbList<MeshSurfacePart*>	parts;
    bool			useownvalidation;
    int				nrcolparts;
    int				sidesize;
};



#endif
    
