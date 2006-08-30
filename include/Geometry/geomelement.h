#ifndef geomelement_h
#define geomelement_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: geomelement.h,v 1.8 2006-08-30 13:59:35 cvskris Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "ranges.h"

typedef int64 GeomPosID;
namespace Geometry
{

class Element : public CallBacker
{
public:
    				Element();
    virtual			~Element();
    virtual void		getPosIDs(TypeSet<GeomPosID>&,
	   				  bool noudf=true) const	= 0;
    virtual IntervalND<float>	boundingBox(bool approx) const;
    virtual Element*		clone() const				= 0;
    virtual const char*		errMsg() const;

    virtual Coord3	getPosition(GeomPosID) const			= 0;
    virtual bool	setPosition(GeomPosID,const Coord3&)		= 0;
    virtual bool	isDefined(GeomPosID) const 			= 0;

    virtual bool	isChanged() const		{ return ischanged_; }
    virtual void	resetChangedFlag()		{ ischanged_=false; }

    virtual void	trimUndefParts()		{}

    CNotifier<Element,const TypeSet<GeomPosID>*>	movementnotifier;
    CNotifier<Element,const TypeSet<GeomPosID>*>	nrpositionnotifier;

    void			blockCallBacks(bool yn,bool flush=true);
    				/*!Block callbacks until further notice.
				   If blocked, lists of added/changed positions
				   will accumulate changes, so they can be
				   flushed when the block is turned off.
				   \param flush is only used when yn is false,
					  and specifies whether a callback
					  with all accumulted changes will
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

    BufferString&		errmsg();

private:

    BufferString*		errmsg_;
};

};

#endif
