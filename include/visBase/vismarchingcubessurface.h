#ifndef vismarchingcubessurface_h
#define vismarchingcubessurface_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		August 2006
 RCS:		$Id: vismarchingcubessurface.h,v 1.13 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "samplingdata.h"

template <class T> class SamplingData;
class MarchingCubesSurface;
class ExplicitMarchingCubesSurface;
class SoShapeHints;
class TaskRunner;

namespace visBase
{

class GeomIndexedShape;

/*!Class to display ::MarchingCubesSurface or body sections. */

mClass MarchingCubesSurface : public VisualObjectImpl
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

    void				touch(bool forall,TaskRunner* =0);
    void				renderOneSide( int side );
    					/*!< 0 = visisble from both sides.
					     1 = visisble from positive side
					    -1 = visisble from negative side. */
					
    					//For body section display only. 
    char				enabledSection() const;
    void				enableSection(char);
    					/*!< -1: display the whole isosurface.
					      0: display section along inline.
					      1: display section along crline.
					      2: display section along z. */
    void				setSectionPosition(float);
    float				getSectionPosition();
    void				setBoxBoudary(float x,float y,float z);

protected:
    					~MarchingCubesSurface();
    void				updateHints();
    void				updateDisplayRange();

    char				displaysection_;
    float				sectionlocation_;
    StepInterval<float>			xrg_;
    StepInterval<float>			yrg_;
    StepInterval<float>			zrg_;

    SoShapeHints*			hints_;
    GeomIndexedShape*			shape_;

    ExplicitMarchingCubesSurface*	surface_;
    char				side_;
};

};
	
#endif
