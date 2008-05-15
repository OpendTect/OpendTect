#ifndef faultsticksurface_h
#define faultsticksurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        K. Tingdahl / J.C. Glas
Date:          September 2007
RCS:           $Id: faultsticksurface.h,v 1.9 2008-05-15 22:04:57 cvskris Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "rowcolsurface.h"

namespace Geometry
{

class FaultStickSurface : public RowColSurface
{
public:
    			FaultStickSurface();
    			~FaultStickSurface();
    bool		isEmpty() const		{ return !sticks_.size(); }
    Element*		clone() const;

    bool		insertStick(const Coord3& firstpos,
				    const Coord3& editnormal,int stickidx=0);
    bool		removeStick(int stickidx);

    bool		insertKnot(const RCol&,const Coord3&);
    bool		removeKnot(const RCol&);

    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange(int stickidx) const;

    bool		setKnot(const RCol&,const Coord3&);
    Coord3		getKnot(const RCol&) const;
    bool		isKnotDefined(const RCol&) const;

    bool		areSticksVertical() const;
    const Coord3&	getEditPlaneNormal(int stickidx) const;				
    enum ChangeTag	{StickChange=__mUndefIntVal+1,StickInsert,StickRemove};
    
    			// To be used by surface reader only
    void		addUdfRow(int stickidx,int firstknotnr,int nrknots);
    void		addEditPlaneNormal(const Coord3&);

protected:

    bool			sticksvertical_;

    int				firstrow_;

    ObjectSet<TypeSet<Coord3> >	sticks_;
    TypeSet<int>		firstcols_;
    
    TypeSet<Coord3>		editplanenormals_;
};

};

#endif
