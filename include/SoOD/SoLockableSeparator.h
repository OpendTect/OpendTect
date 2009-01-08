#ifndef SoLockableSeparator_h
#define SoLockableSeparator_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          October 2008
 RCS:           $Id: SoLockableSeparator.h,v 1.3 2009-01-08 09:48:12 cvsnanne Exp $
________________________________________________________________________

-*/


#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/threads/SbRWMutex.h>

#define mImplFunc( func, action )	\
func( action* a )			\
{					\
    lock.readLock();			\
    inherited::func( a );		\
    lock.readUnlock();			\
}		


/*!A separator with a lock, that is readlocked during all traversals. */

class COIN_DLL_API SoLockableSeparator : public SoSeparator
{
    typedef SoSeparator inherited;
    SO_NODE_HEADER(SoLockableSeparor);

public:
    static void	initClass(void);
		SoLockableSeparator();

    void	mImplFunc( doAction, SoAction );
    void	mImplFunc( GLRender, SoGLRenderAction );
    void	mImplFunc( GLRenderBelowPath, SoGLRenderAction );
    void	mImplFunc( GLRenderInPath, SoGLRenderAction );
    void	mImplFunc( GLRenderOffPath, SoGLRenderAction );
    void	mImplFunc( callback, SoCallbackAction );
    void	mImplFunc( getBoundingBox, SoGetBoundingBoxAction );
    void	mImplFunc( getMatrix, SoGetMatrixAction );
    void	mImplFunc( handleEvent, SoHandleEventAction );
    void	mImplFunc( rayPick, SoRayPickAction );
    void	mImplFunc( search, SoSearchAction );
    void	mImplFunc( getPrimitiveCount, SoGetPrimitiveCountAction );
    void	mImplFunc( audioRender, SoAudioRenderAction );

    SbRWMutex	lock;

protected:

  		~SoLockableSeparator()		{}
};

#endif
