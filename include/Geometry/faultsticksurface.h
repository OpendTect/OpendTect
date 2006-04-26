#ifndef faultsticksurface_h
#define faultsticksurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        A.H. Bril
Date:          23-10-1996
Contents:      Ranges
RCS:           $Id: faultsticksurface.h,v 1.2 2006-04-26 21:04:10 cvskris Exp $
________________________________________________________________________

-*/

#include "rowcolsurface.h"

template <class T> class Array2D;

namespace Geometry
{

class FaultStickSurface : public RowColSurface
{
public:
    			FaultStickSurface();
    			~FaultStickSurface();

    bool		insertStick(int stick);
    bool		removeStick(int stick);

    bool		setKnot(const RCol&,const Coord3&);
    bool		unsetKnot(const RCol&);
    Coord3		getKnot(const RCol&) const;
    bool		isKnotDefined(const RCol&) const;

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
