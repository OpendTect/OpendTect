#ifndef SoKrisSurface_h
#define SoKrisSurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoKrisSurface.h,v 1.2 2004-11-16 10:07:44 kristofer Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoShape.h"
#include "Inventor/fields/SoSFShort.h"
#include "Inventor/fields/SoMFInt32.h"
#include "Inventor/fields/SoMFVec3f.h"
#include "Inventor/fields/SoSFInt32.h"
#include "Inventor/fields/SoSFBool.h"
#include "Inventor/fields/SoMFShort.h"
#include "Inventor/lists/SbList.h"
#include "Inventor/nodes/SoSubNode.h"

class SbVec3f;
class MeshSurfacePart;

/*!
A class for displaying inexed mesh-surfaces


meshStyle

0    ------	Auto - the tesselation is based on the data
     |\XX/|
     |X\/X|
     |X/\X|
     |/XX\|
     ------

1    ------
     |\XXX|
     |X\XX|
     |XX\X|
     |XXX\|
     ------

2    ------
     |XXX/|
     |XX/X|
     |X/XX|
     |/XXX|
     ------

3    ------
     |\   |
     |X\  |
     |XX\ |
     |XXX\|
     ------

4    ------
     |   /|
     |  /X|
     | /XX|
     |/XXX|
     ------

5    ------
     |\XXX|
     | \XX|
     |  \X|
     |   \|
     ------

6    ------
     |XXX/|
     |XX/ |
     |X/  |
     |/   |
     ------

7    ------
     |    |
     |    |
     |    |
     |    |
     ------

*/


class SoKrisSurface : public SoShape
{
    SO_NODE_HEADER(SoKrisSurface);
public:
    SoMFVec3f		coordinates;
    			/* If the x component of a coordinate less than
			   1e29, the coordinate is considered defined */
    SoMFInt32		materialIndex;
    SoMFShort		meshStyle;
    			
    SoSFShort		nrColumns;
    SoSFShort		brickSize;
    SoSFShort		resolution;
    SoSFBool		wireframe;

    void		insertColumns(bool before);
    			/*!<Inserts 2^bricksize before or after the current
			    mesh. The function will fill the new space with
			    -1s and will not trigger a invalidation of caches.
			*/
    void		insertRowsBefore();
    			/*!<Inserts 2^bricksize before the current mesh. The
			    function will fill the new space with -1s and
			    will not trigger a invalidation of caches.
			*/

    void		turnOnOwnValidation(bool yn);
    void		touch( int, int, bool undef );

    void		generatePrimitives(SoAction*) {}
    void		computeBBox(SoAction*, SbBox3f&, SbVec3f&);
    void		rayPick( SoRayPickAction*);
    void 		GLRender(SoGLRenderAction*);
    void		getBoundingBox(SoGetBoundingBoxAction *action);

    int			getIndex( int, int ) const;

    			SoKrisSurface();
    static void		initClass();

    int			nrRows() const;

protected:
    void		notify( SoNotList* );

    void		adjustNrOfParts();

    SbList<MeshSurfacePart*>	parts;
    bool			useownvalidation;
    int				nrcolparts;
    int				sidesize;
};



#endif
    
