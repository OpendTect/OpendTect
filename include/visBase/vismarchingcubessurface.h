#ifndef vismarchingcubessurface_h
#define vismarchingcubessurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		August 2006
 RCS:		$Id: vismarchingcubessurface.h,v 1.4 2007-09-07 15:31:05 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"

template <class T> class SamplingData;
class MarchingCubesSurface;
class ExplicitMarchingCubesSurface;
class SoIndexedTriangleStripSet;
class SoShapeHints;

namespace visBase
{

class Coordinates;
class Normals;

/*!Class to display ::IsoSurfaces. */

class MarchingCubesSurface : public VisualObjectImpl
{
public:
    static MarchingCubesSurface*	create()
					mCreateDataObj(MarchingCubesSurface);

    void				setRightHandSystem(bool);
    bool				isRightHandSystem() const;

    void				setSurface(::MarchingCubesSurface&);
    ::MarchingCubesSurface*		getSurface();
    const ::MarchingCubesSurface*	getSurface() const;

    void				setScales(const SamplingData<float>&,
	    					  const SamplingData<float>&,
						  const SamplingData<float>&);
    const SamplingData<float>&		getScale(int dim) const;

    void				touch();
    void				renderOneSide( int side );
    					/*!< 0 = visisble from both sides.
					     1 = visisble from positive side
					    -1 = visisble from negative side. */

protected:
    					~MarchingCubesSurface();
    void				updateHints();

    Coordinates*				coords_;
    Normals*					normals_;
    SoShapeHints*				hints_;
    ObjectSet<SoIndexedTriangleStripSet>	triangles_;

    ExplicitMarchingCubesSurface*		surface_;
    char					side_;
};

};
	
#endif
