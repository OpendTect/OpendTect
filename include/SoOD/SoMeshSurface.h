#ifndef SoMeshSurface_h
#define SoMeshSurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoMeshSurface.h,v 1.1 2003-09-30 13:00:57 kristofer Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoGroup.h"
#include "Inventor/fields/SoSFShort.h"
#include "Inventor/fields/SoMFInt32.h"
#include "Inventor/fields/SoSFInt32.h"
#include "Inventor/SbLinear.h"
#include "Inventor/lists/SbList.h"
#include "Inventor/nodes/SoSubNode.h"

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

    void			setPos(int row,int col,const SbVec3f&,
	    				const SbVec2s&,const SbVec2s&);
    void			removePos(int row,int col);
    void			updateTextureCoords();

    void			allocSpace(int rowstart,int rowstop,
	    			    	   int colstart,int colstop);

    void			turnOnWireFrame(bool yn);
    bool			isWireFrameOn() const;
    

    static inline SbBool	isUndefined(float);
    static inline SbBool	isUndefined(const SbVec3f&);
    static inline float		undefVal();

    virtual void		GLRender(SoGLRenderAction*);
protected:
    void			setUpObject();

    int				getSquareIndex( int row, int col ) const;
    				/*!< \note
				     Lock the mutex before access and dont
				     unlock until you are finished with the
				     results (so it doesn't become invalid).
				*/

    SoMeshSurfaceSquare*	getSquare( int squareindex );
    const SoMeshSurfaceSquare*	getSquare( int squareindex ) const;

    void			makeFirstSquare( int row, int col );
    void			addSquareRow( bool start );
    void			addSquareCol( bool start );

    SbMutex*			setupMutex;
    				//Protects firstrow, firstcol, nrcolsquares,
    				//squaresize, and the children (add/remove)

    int				firstrow;
    int				firstcol;
    int				nrcolsquares;
    int				squaresize;
    int				nrRows() const;
    
    SoFieldSensor*		partSizePowerSensor;
    static void			partSizePowerCB( void*, SoSensor* );

    
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
    
