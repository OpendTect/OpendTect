#ifndef SoMeshSurface_h
#define SoMeshSurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoMeshSurface.h,v 1.4 2003-10-09 14:13:08 nanne Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoGroup.h"
#include "Inventor/fields/SoSFShort.h"
#include "Inventor/fields/SoMFInt32.h"
#include "Inventor/fields/SoSFInt32.h"
#include "Inventor/SbLinear.h"
#include "Inventor/lists/SbList.h"
#include "Inventor/nodes/SoSubNode.h"

class SoCallbackList;
class SbCondVar;
class SoFieldSensor;
class SoMeshSurfaceSquare;
class SoMeshSurfaceBrick;
class SbMutex;
class SoSensor;
class SoFieldSensor;
class SbThread;
class SbVec3f;
class SbVec2s;

class SoMeshSurface;

typedef void SoMeshSurfaceCB( void* data, SoMeshSurface* );

/*!\brief
Is a Mesh-based surface with multiple resolution. The surface is set up with
setPos( row, col, SbVec3f ). Both negative and positive row and col are allowed. 
The surface is separated into many squares, where the size of the squares
are determined by 2^partSizePower.

If textures are used, you must specify the range of row-col that the
texture covers.

It is possible to have holes int the surface and iregular borders. That
is specified by setting the coord to undefValue.
*/


class SoMeshSurface : public SoGroup
{
    typedef SoGroup inherited;
    SO_NODE_HEADER(SoMeshSurface);
public:
    				SoMeshSurface();
    static void			initClass();

    SoSFShort			whichResolution;
    				//!< -1 = auto
    SoSFShort			partSizePower;

    SbVec3f			getPos(int,int);
    void			setPos(int row,int col,const SbVec3f&);
    void			removePos(int row,int col);

    void			setTextureRange(int firstrow,int firstcol,
	    					int lastrow,int lastcol);

    void			allocSpace(int rowstart,int rowstop,
	    			    	   int colstart,int colstop);

    void			turnOnWireFrame(bool yn);
    bool			isWireFrameOn() const;

    void			addPickCB( SoMeshSurfaceCB*, void* data = 0 );
    void			removePickCB( SoMeshSurfaceCB*, void* data = 0);
    void			getPickedRowCol(int& row,int& col) const;

    static inline SbBool	isUndefined(float);
    static inline SbBool	isUndefined(const SbVec3f&);
    static inline float		undefVal();

    virtual void		GLRender(SoGLRenderAction*);

protected:
    void			setUpObject();

    int				getSquareIndex(int row,int col) const;
    				/*!< \note
				     Lock the mutex before access and dont
				     unlock until you are finished with the
				     results (so it doesn't become invalid).
				*/

    SoMeshSurfaceSquare*	getSquare(int squareindex);
    const SoMeshSurfaceSquare*	getSquare(int squareindex) const;

    void			makeFirstSquare(int row,int col);
    void			addSquareRow(bool start);
    void			addSquareCol(bool start);

    SbMutex*			setupMutex;
    				//Protects firstrow, firstcol, nrcolsquares,
    				//squaresize, and the children (add/remove)

    int				firstrow;
    int				firstcol;
    int				nrcolsquares;
    int				squaresize;
    int				nrRows() const;

    int				texturefirstrow;
    int				texturefirstcol;
    int				texturelastrow;
    int				texturelastcol;
    bool			texturerangeisset;
    
    SoFieldSensor*		partSizePowerSensor;
    static void			partSizePowerCB( void*, SoSensor* );

    SoCallbackList*		pickcallbacks;
    int				pickedrow, pickedcol;
    static void			pickCB( void*, SoMeshSurfaceSquare* );

    
    void				stopThreads();
    void				startThreads(int nrthreads);

    SbList<SoMeshSurfaceBrick*>		creationquebricks;

    SbMutex*				creatorqueMutex;
    SbCondVar*				creationcondvar;
    SbBool				weAreStopping;
    static void*			creationFunc(void*);
    SbList<SbThread*>			threads;

    SbList<SoMeshSurfaceSquare*>	rendersquares;

					~SoMeshSurface();
};


inline SbBool SoMeshSurface::isUndefined(float val)
{
    return ( (val>9.99999e29) && (val<1.00001e30) );
}


inline SbBool SoMeshSurface::isUndefined(const SbVec3f& vec)
{ return isUndefined(vec[0]) || isUndefined(vec[1]) || isUndefined(vec[2]); }


inline float SoMeshSurface::undefVal()
{
    return 1e30;
}


#endif
    
