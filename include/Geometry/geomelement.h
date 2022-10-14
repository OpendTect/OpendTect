#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    virtual 			~Iterator();
    virtual GeomPosID		next()			= 0;
				//!<returs -1 if not valid
protected:
				Iterator();
};


mExpClass(Geometry) Element : public CallBacker
{ mODTextTranslationClass(Element);
public:
				Element();
				Element(const Element&);
    virtual			~Element();

    Element&			operator =(const Element&);

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
    bool			blockcbs_		= false;
    bool			ischanged_		= false;
    TypeSet<GeomPosID>		nrposchbuffer_;
    TypeSet<GeomPosID>		movementbuffer_;
    void			triggerMovement(const TypeSet<GeomPosID>&);
    void			triggerMovement(const GeomPosID&);
    void			triggerMovement();
    void			triggerNrPosCh(const TypeSet<GeomPosID>&);
    void			triggerNrPosCh(const GeomPosID&);
    void			triggerNrPosCh();

    uiString&			errmsg();

private:

    Threads::Lock		poschglock_;
    Threads::Lock		movementlock_;
    uiString*			errmsg_			= nullptr;
};

} // namespace Geometry
