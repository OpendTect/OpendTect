#ifndef impbodyplaneintersect_h
#define impbodyplaneintersect_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Yuancheng Liu
Date:          December 2011
RCS:           $Id$
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "arraynd.h"
#include "cubesampling.h"
#include "position.h"

class Coord3List;

namespace Geometry
{

class IndexedShape;


mExpClass(Geometry) ImplicitBodyPlaneIntersector
{
public:
				ImplicitBodyPlaneIntersector(
					const Array3D<float>& bodyarray,
					const CubeSampling& bodyrg,
					float bodythreshold,
					char dim,float icz,IndexedShape&);
				//Make sure bodyarray has the same size as rg.
				//dim is the plane orientation
				//dim=0 icz=inline is the inline position
				//dim=1 icz=crossline is the crossline position
				//dim=2 icz=z is the z position

    bool			compute();	

protected:

    IndexedShape&		output_;

    const Array3D<float>&	arr_;
    const CubeSampling&		cs_;
    float			threshold_;
    char			dim_;
    float			inlcrlz_;
};


};


#endif

