#ifndef geomelement_h
#define geomelement_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: geomelement.h,v 1.2 2005-01-13 12:01:45 nanne Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "ranges.h"

typedef int64 GeomPosID;

namespace Geometry
{

class Element : public CallBackClass
{
public:
    				Element();
    virtual			~Element();
    virtual void		getPosIDs( TypeSet<GeomPosID>& ) const	= 0;
    virtual IntervalND<float>	boundingBox(bool approx=true) const;
    virtual Element*		clone() const				= 0;
    virtual const char*		errMsg() const;

    virtual Coord3		getPosition( GeomPosID ) const		= 0;
    virtual bool		setPosition( GeomPosID, const Coord3& )	= 0;
    virtual bool		isDefined( GeomPosID ) const 		= 0;

    virtual bool		isChanged() const { return ischanged; }
    virtual void		resetChangedFlag() { ischanged=false; }

    const Coord&		scaling() const { return scaling_; }
    void			setScaling(const Coord3& ns) { scaling_ = ns; }

    CNotifier<Element, const TypeSet<GeomPosID>*>	movementnotifier;
    CNotifier<Element, const TypeSet<GeomPosID>*>	nrpositionnotifier;

protected:
    void			triggerMovement( const TypeSet<GeomPosID>& );
    void			triggerMovement( const GeomPosID& );
    void			triggerMovement();
    void			triggerNrPosCh( const TypeSet<GeomPosID>& );
    void			triggerNrPosCh( const GeomPosID& );
    void			triggerNrPosCh();
    bool			ischanged;

    BufferString&		errmsg();

private:

    BufferString*		errmsg_;
    Coord3			scaling_;
};

};

#endif
