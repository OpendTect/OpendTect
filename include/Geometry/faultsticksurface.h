#ifndef faultsticksurface_h
#define faultsticksurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        K. Tingdahl
Date:          23-10-1996
RCS:           $Id: faultsticksurface.h,v 1.3 2007-09-14 15:37:59 cvsjaap Exp $
________________________________________________________________________

-*/

#include "rowcolsurface.h"

namespace Geometry
{

class FaultStickSurface : public RowColSurface
{
public:
    			FaultStickSurface();
    			~FaultStickSurface();

    bool		insertStick(const Coord3& firstpos,
				    const Coord3& editnormal,int sticknr=0);
    bool		removeStick(int sticknr);

    bool		insertKnot(const RCol&,const Coord3&);
    bool		removeKnot(const RCol&);

    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange(int row) const;

    bool		setKnot(const RCol&,const Coord3&);
    Coord3		getKnot(const RCol&) const;
    bool		isKnotDefined(const RCol&) const;

    bool		areSticksVertical() const;
    const Coord3&	getEditPlaneNormal(int sticknr) const;				

protected:

    bool			sticksvertical_;

    int				firstrow_;

    ObjectSet<TypeSet<Coord3> >	sticks_;
    TypeSet<int>		firstcols_;
    
    TypeSet<Coord3>		editplanenormals_;
};

};

#endif
