#ifndef SoLockableSeparator_h
#define SoLockableSeparator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          October 2008
 RCS:           $Id: SoLockableSeparator.h,v 1.6 2009-07-22 16:01:19 cvsbert Exp $
________________________________________________________________________

-*/


#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/threads/SbRWMutex.h>

#include "soodbasic.h"

#define mImplFunc( func, action )		\
func( action* a )				\
{						\
    if ( !lock.tryReadLock() )			\
	return;					\
    						\
    inherited::func( a );			\
    lock.readUnlock();				\
}		


/*!A separator with a lock, that is readlocked during all traversals. */

mClass SoLockableSeparator : public SoSeparator
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
