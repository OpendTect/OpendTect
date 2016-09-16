#pragma once
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Yuancheng Liu
Date:          March 2010
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "gendefs.h"

class Coord3List;

namespace Geometry
{

class BinIDSurface;
class IndexedShape;
class ExplFaultStickSurface;


mExpClass(Geometry) FaultBinIDSurfaceIntersector
{
public:
				FaultBinIDSurfaceIntersector(float horshift,
					const BinIDSurface&, 
					const ExplFaultStickSurface&,
					Coord3List&);
				~FaultBinIDSurfaceIntersector()	{}

    void			compute();	
				
				//The shape is optional, if not set, we still
				//compute intersections, stored in crdlist_    
    void			setShape(const IndexedShape&);
    const IndexedShape*		getShape(bool takeover=true);

protected:

    float			zshift_;
    Coord3List&			crdlist_;
    const BinIDSurface&		surf_;
    const IndexedShape*		output_;
    const ExplFaultStickSurface& eshape_;

private:
    void			sortPointsToLine(TypeSet<Coord3>&,
						 TypeSet<Coord3>&);
    const Coord3	        findNearestPoint(const Coord3&,
						 TypeSet<Coord3>&);
    bool			findMin(TypeSet<Coord3>&,int&,bool);
    int				optimizeOrder(TypeSet<Coord3>&);

};


};
