#ifndef pickretriever_h
#define pickretriever_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          August 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "callback.h"
#include "refcount.h"

class Coord3;
namespace PosInfo { class GeomID; }

/*!Interface to when an application wants a pick somewere in a 3D environment.
   There should normally only be one instance in memory, and that should
   be accessed via PickRetriever::getInstance(). */

mExpClass(General) PickRetriever : public CallBacker
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
    virtual const PosInfo::GeomID& getGeomID() const			= 0;
    virtual int			getTrcNr() const			= 0;
    virtual int			getSceneID() const			= 0;
    virtual const TypeSet<int>&	getPickedObjIDs() const			= 0;

    static PickRetriever*	getInstance();
    				/*!<Main access function. */

    static void			setInstance(PickRetriever*);
    				/*!<Should normally only be called from
				    application initiation. */
protected:
    static RefMan<PickRetriever> instance_;
};


#endif

