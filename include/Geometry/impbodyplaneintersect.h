#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "gendefs.h"
template <class T> class Array3D;
class Coord3List;
class TrcKeyZSampling;

namespace Geometry
{

class IndexedShape;


mExpClass(Geometry) ImplicitBodyPlaneIntersector
{
public:
				ImplicitBodyPlaneIntersector(
					const Array3D<float>& bodyarray,
					const TrcKeyZSampling& bodyrg,
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
    const TrcKeyZSampling&		tkzs_;
    float			threshold_;
    char			dim_;
    float			inlcrlz_;
};

} // namespace Geometry
