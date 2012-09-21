#ifndef vismarchingcubessurface_h
#define vismarchingcubessurface_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		August 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"
#include "samplingdata.h"

template <class T> class SamplingData;
class MarchingCubesSurface;
class ExplicitMarchingCubesSurface;
class TaskRunner;

namespace visBase
{

class GeomIndexedShape;

/*!Class to display ::MarchingCubesSurface or body sections. */

mClass(visBase) MarchingCubesSurface : public VisualObjectImpl
{
public:
    static MarchingCubesSurface*	create()
					mCreateDataObj(MarchingCubesSurface);

    void				setSurface(::MarchingCubesSurface&,
	    					   TaskRunner*);
    ::MarchingCubesSurface*		getSurface();
    const ::MarchingCubesSurface*	getSurface() const;

    void				setRightHandSystem(bool);

    void				setScales(const SamplingData<float>&,
	    					  const SamplingData<float>&,
						  const SamplingData<float>&);
    const SamplingData<float>&		getScale(int dim) const;

    void			touch(bool forall,TaskRunner* =0);
    void			renderOneSide( int side );
    				/*!< 0 = visisble from both sides.
				     1 = visisble from positive side
				    -1 = visisble from negative side. */
					
    				//For body section display only. 
    char			enabledSection() const;
    void			enableSection(char);
    				/*!< -1: display the whole isosurface.
				      0: display section along inline.
				      1: display section along crline.
				      2: display section along z. */
    void			setSectionPosition(float);
    float			getSectionPosition();
    void			setBoxBoundary(float x,float y,float z);

    GeomIndexedShape*		getShape()		{ return shape_; }

protected:
    					~MarchingCubesSurface();
    void				updateHints();
    void				updateDisplayRange();
    static const char*			sKeyCoordIndex() { return "CoordIndex";}

    char				displaysection_;
    float				sectionlocation_;
    StepInterval<float>			xrg_;
    StepInterval<float>			yrg_;
    StepInterval<float>			zrg_;

    GeomIndexedShape*			shape_;

    ExplicitMarchingCubesSurface*	surface_;
};

};
	
#endif

