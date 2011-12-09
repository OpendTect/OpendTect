#ifndef impbodyplaneintersect_h
#define impbodyplaneintersect_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Yuancheng Liu
Date:          December 2011
RCS:           $Id: impbodyplaneintersect.h,v 1.1 2011-12-09 22:41:58 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "arraynd.h"
#include "cubesampling.h"
#include "position.h"

class Coord3List;

namespace Geometry
{

class IndexedShape;


mClass ImplicitBodyPlaneIntersector
{
public:
				ImplicitBodyPlaneIntersector(
					const Array3D<float>& bodyarray,
					const CubeSampling& bodyrg,
					float bodythreshold,
					char dim,float icz,Coord3List&);
				//Make sure bodyarray has the same size as rg.
				//dim is the plane orientation
				//dim=0 icz=inline is the inline position
				//dim=1 icz=crossline is the crossline position
				//dim=2 icz=z is the z position

    bool			compute();	
				
    void			setShape(const IndexedShape&);
    const IndexedShape*		getShape(bool takeover=true);

protected:

    Coord3List&			crdlist_;
    const IndexedShape*		output_;

    const Array3D<float>&	arr_;
    const CubeSampling&		cs_;
    float			threshold_;
    char			dim_;
    float			inlcrlz_;
};


};


#endif
