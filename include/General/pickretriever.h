#ifndef pickretriever_h
#define pickretriever_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          August 2006
 RCS:           $Id: pickretriever.h,v 1.2 2007-08-17 09:27:05 cvsnanne Exp $
________________________________________________________________________

-*/

#include "zaxistransform.h"

/*!Interface to when an application wants a pick somewere in a 3D environment.
   There should normally only be one instance in memory, and that should
   be accessed via PickRetriever::getInstance(). */

class PickRetriever : public CallBacker
{ mRefCountImpl(PickRetriever);
public:
    				PickRetriever();

    virtual bool		enable(const TypeSet<int>* allowedsc)	= 0;
    				/*!<Sets the object in a state where it's
				    retrieving picks.
				    \note if allowedsc is empty or null,
				 	  picks are allowed in all scenes. */
    virtual NotifierAccess*	finished()				= 0;
    				/*!<Triggers when it does not look for pick
				    any longer. The outcome can be retrieved
				    bu success(), getPos() and getSceneID(). */
    virtual void		reset()					= 0;
    virtual bool		success() const				= 0;
    virtual bool		waiting() const				= 0;
    virtual const Coord3&	getPos() const				= 0;
    virtual int			getSceneID() const			= 0;

    static PickRetriever*	getInstance();
    				/*!<Main access function. */

    static void			setInstance(PickRetriever*);
    				/*!<Should normally only be called from
				    application initiation. */
protected:
    static RefMan<PickRetriever> instance_;
};


#endif
