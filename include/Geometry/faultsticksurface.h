#ifndef faultsticksurface_h
#define faultsticksurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: faultsticksurface.h,v 1.1 2006-03-20 16:25:37 cvskris Exp $
________________________________________________________________________

-*/

#include "rowcol.h"
#include "geomelement.h"

template <class T> class Array2D;

namespace Geometry
{

class FaultStickSurface : public Element
{
public:
    			FaultStickSurface();
    			~FaultStickSurface();
    ParametricSurface*	clone() const;
    void		getPosIDs(TypeSet<GeomPosID>&,bool=true) const;

    bool		insertStick(int stick);
    bool		removeStick(int stick);

    StepInterval<int>	knotRange(int stick) const;
    StepInterval<int>	stickRange() const;

    bool		setKnot(const RCol&,const Coord3&);
    bool		unsetKnot(const RCol&);
    Coord3		getKnot(const RCol&) const;
    bool		isKnotDefined(const RCol&) const;

    Coord3		getPosition(GeomPosID pid) const;
    bool		setPosition(GeomPosID pid,const Coord3&);
    bool		isDefined(GeomPosID pid) const;

    Notifier<FaultStickSurface>	startsRebuilding;
    Notifier<FaultStickSurface>	finishRebuilding;

    const float*		getTriangleCoords() const;
    int				nrTriangleCoords() const;
    Notifier<FaultStickSurface>	coordChange;
    				/*!<It is guaranteed that the configuration,
				    i.e. nr of trianges or their connections
				    has not been changed. */
 
    const int*			getTriangleFanIndices() const;
    int				nrTriangeFanIndices() const;

    float			distanceToPoint( const Coord3& ) const;

protected:
    void			rebuildTriangles();

    bool			stickishorizontal_;

    bool			autorebuild_;
    int				firstcolumn_;

    ObjectSet<TypeSet<Coord3> >	columns_;
    TypeSet<int>		firstrows_;

    TypeSet<float>		tricoords_;
    TypeSet<int>		trifanindices_;
};

};

#endif
