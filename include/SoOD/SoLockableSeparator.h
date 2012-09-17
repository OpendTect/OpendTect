#ifndef SoLockableSeparator_h
#define SoLockableSeparator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          October 2008
 RCS:           $Id: SoLockableSeparator.h,v 1.7 2009/08/20 01:02:52 cvskris Exp $
________________________________________________________________________

-*/


#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/threads/SbRWMutex.h>

#include "soodbasic.h"

#define mImplFunc( func, action, inherited )	\
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
    SO_NODE_HEADER(SoLockableSeparor);

public:
    static void	initClass(void);
		SoLockableSeparator();

    void	mImplFunc( doAction, SoAction, SoSeparator );
    void	mImplFunc( GLRender, SoGLRenderAction, SoSeparator );
    void	mImplFunc( GLRenderBelowPath, SoGLRenderAction, SoSeparator );
    void	mImplFunc( GLRenderInPath, SoGLRenderAction, SoSeparator );
    void	mImplFunc( GLRenderOffPath, SoGLRenderAction, SoSeparator );
    void	mImplFunc( callback, SoCallbackAction, SoSeparator );
    void	mImplFunc( getBoundingBox, SoGetBoundingBoxAction, SoSeparator);
    void	mImplFunc( getMatrix, SoGetMatrixAction, SoSeparator );
    void	mImplFunc( handleEvent, SoHandleEventAction, SoSeparator );
    void	mImplFunc( pick, SoPickAction, SoGroup );
    void	mImplFunc( rayPick, SoRayPickAction, SoSeparator );
    void	mImplFunc( search, SoSearchAction, SoSeparator );
    void	mImplFunc( write, SoWriteAction, SoGroup );
    void	mImplFunc( getPrimitiveCount, SoGetPrimitiveCountAction,
	    		   SoSeparator );
    void	mImplFunc( audioRender, SoAudioRenderAction, SoSeparator );

    SbRWMutex	lock;

protected:

  		~SoLockableSeparator()		{}
};

#endif
