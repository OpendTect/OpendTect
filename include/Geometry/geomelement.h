#pragma once

/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bert
Date:          23-10-1996
Contents:      Ranges
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "binid.h"
#include "coord.h"
#include "integerid.h"
#include "notify.h"
#include "ranges.h"
#include "rowcol.h"
#include "uistring.h"


class GeomPosID : public IntegerID<od_int64>
{
public:

    explicit		GeomPosID( const Pos::IdxPair& rc )
			    : GeomPosID(rc.toInt64())		{}
			GeomPosID( int row, int col )
			    : GeomPosID(RowCol(row,col))	{}
    inline		GeomPosID()				{}

    static inline GeomPosID get( od_int64 i )
				{ return GeomPosID(i); }
    static inline GeomPosID getFromRowCol( const Pos::IdxPair& rc )
				{ return GeomPosID(rc.toInt64()); }
    static inline GeomPosID getFromRowCol( int row, int col )
				{ return GeomPosID(RowCol(row,col).toInt64()); }

    inline RowCol	getRowCol() const
			{ return RowCol::fromInt64(getI()); }
    inline BinID	getBinID() const
			{ return BinID::fromInt64(getI()); }

    inline bool		operator ==( const GeomPosID& oth ) const
			{ return IntegerID<od_int64>::operator ==(oth); }
    inline bool		operator !=( const GeomPosID& oth ) const
			{ return IntegerID<od_int64>::operator !=(oth); }

    static inline GeomPosID getInvalid() { return GeomPosID(-1); }

protected:

    inline		GeomPosID( od_int64 i )
			    : IntegerID<od_int64>(i)	{}
    friend class	TypeSet<GeomPosID>;
};


namespace Geometry
{

/*!Iterator through all positions on an element. */

mExpClass(Geometry) Iterator
{ mODTextTranslationClass(Iterator);
public:
    virtual			~Iterator()		{}
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
    virtual bool	isDefined(GeomPosID) const			= 0;

    virtual bool	isChanged() const		{ return ischanged_; }
    virtual void	resetChangedFlag()		{ ischanged_=false; }

    virtual void	trimUndefParts()		{}

    CNotifier<Element,const TypeSet<GeomPosID>*>&	movementNotifier() const
			{ return const_cast<Element*>(this)->movementnotifier; }
    CNotifier<Element,const TypeSet<GeomPosID>*>&     nrpositionNotifier() const
		      { return const_cast<Element*>(this)->nrpositionnotifier; }

    void			blockCallBacks(bool yn,bool flush=true);
				/*!Block callbacks until further notice.
				   If blocked, lists of added/changed positions
				   will accumulate changes, so they can be
				   flushed when the block is turned off.
				   \param flush specifies whether a callback
					  with all accumulated changes will
					  be triggered.
				*/
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

    CNotifier<Element,const TypeSet<GeomPosID>*>	movementnotifier;
    CNotifier<Element,const TypeSet<GeomPosID>*>	nrpositionnotifier;

private:

    Threads::Lock		poschglock_;
    Threads::Lock		movementlock_;
    uiString*			errmsg_;
};

} // namespace Geometry
