#pragma once

/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "coord.h"
#include "ranges.h"
#include "callback.h"
#include "uistring.h"

typedef od_int64 GeomPosID;

namespace Geometry
{

/*!Iterator through all positions on an element. */

mExpClass(Geometry) Iterator
{ mODTextTranslationClass(Iterator);
public:
    virtual 			~Iterator()		{}
    virtual GeomPosID		next()			= 0;
    				//!<returs -1 if not valid
};


mExpClass(Geometry) Element : public CallBacker
{ mODTextTranslationClass(Element);
public:
    				Element();
    virtual			~Element();
    virtual Iterator*		createIterator() const			= 0;

    virtual void		getPosIDs(TypeSet<GeomPosID>&,
	   				  bool noudf=true) const;
    virtual IntervalND<float>	boundingBox(bool approx) const;
    virtual Element*		clone() const				= 0;
    virtual uiString		errMsg() const;

    virtual Coord3	getPosition(GeomPosID) const			= 0;
    virtual bool	setPosition(GeomPosID,const Coord3&)		= 0;
    virtual bool	isDefined(GeomPosID) const 			= 0;

    virtual bool	isChanged() const		{ return ischanged_; }
    virtual void	resetChangedFlag()		{ ischanged_=false; }

    virtual void	trimUndefParts()		{}

    CNotifier<Element,const TypeSet<GeomPosID>*>	movementnotifier;
    CNotifier<Element,const TypeSet<GeomPosID>*>	nrpositionnotifier;

    				/*!Block callbacks until further notice.
				   If blocked, lists of added/changed positions
				   will accumulate changes, so they can be
				   flushed when the block is turned off.
				   \param yn
				   \param flush specifies whether a callback
					  with all accumulated changes will
					  be triggered.
				*/
    void			blockCallBacks(bool yn,bool flush=true);
    bool			blocksCallBacks() const { return blockcbs_; }

protected:
    bool			blockcbs_;
    TypeSet<GeomPosID>		nrposchbuffer_;
    TypeSet<GeomPosID>		movementbuffer_;
    void			triggerMovement(const TypeSet<GeomPosID>&);
    void			triggerMovement(const GeomPosID&);
    void			triggerMovement();
    void			triggerNrPosCh(const TypeSet<GeomPosID>&);
    void			triggerNrPosCh(const GeomPosID&);
    void			triggerNrPosCh();
    bool			ischanged_;

    uiString&			errmsg();

private:

    Threads::Lock		poschglock_;
    Threads::Lock		movementlock_;
    uiString*			errmsg_;
};

} // namespace Geometry

