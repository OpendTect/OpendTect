#ifndef vismarchingcubessurface_h
#define vismarchingcubessurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		August 2006
 RCS:		$Id: vismarchingcubessurface.h,v 1.10 2007-10-30 20:24:23 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "visobject.h"

template <class T> class SamplingData;
class MarchingCubesSurface;
class ExplicitMarchingCubesSurface;
class SoShapeHints;

namespace visBase
{

class GeomIndexedShape;

/*!Class to display ::MarchingCubesSurface. */

class MarchingCubesSurface : public VisualObjectImpl
{
public:
    static MarchingCubesSurface*	create()
					mCreateDataObj(MarchingCubesSurface);

    void				setSurface(::MarchingCubesSurface&);
    ::MarchingCubesSurface*		getSurface();
    const ::MarchingCubesSurface*	getSurface() const;

    void				setRightHandSystem(bool);

    void				setScales(const SamplingData<float>&,
	    					  const SamplingData<float>&,
						  const SamplingData<float>&);
    const SamplingData<float>&		getScale(int dim) const;

    void				touch(bool forall);
    void				renderOneSide( int side );
    					/*!< 0 = visisble from both sides.
					     1 = visisble from positive side
					    -1 = visisble from negative side. */

protected:
    					~MarchingCubesSurface();
    void				updateHints();

    SoShapeHints*				hints_;
    GeomIndexedShape*				shape_;

    ExplicitMarchingCubesSurface*		surface_;
    char					side_;
};

};
	
#endif
