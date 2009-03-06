#ifndef vismarchingcubessurfacedisplay_h
#define vismarchingcubessurfacedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismarchingcubessurfacedisplay.h,v 1.14 2009-03-06 16:09:51 cvskris Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "ranges.h"

class MarchingCubesSurfaceEditor;
template <class T> class Array3D;

namespace visBase
{
    class BoxDragger;
    class Dragger;
    class Ellipsoid;
    class IndexedPolyLine;
    class MarchingCubesSurface;
    class PickStyle;
    class Transformation;
    class InvisibleLineDragger;
};


namespace EM { class MarchingCubesSurface; }


namespace visSurvey
{
class Scene;

/*!\brief Used for displaying welltracks, markers and logs


*/

mClass MarchingCubesDisplay : public visBase::VisualObjectImpl,
			     public visSurvey::SurveyObject
{
public:
    static MarchingCubesDisplay*create()
				mCreateDataObj(MarchingCubesDisplay);

    MultiID		getMultiID() const;
    bool		isInlCrl() const	{ return true; }

    bool		hasColor() const	{ return true; }
    Color		getColor() const;
    void		setColor(Color);
    bool		allowMaterialEdit() const { return true; }
    NotifierAccess*	materialChange();

    bool		canHandleColTabSeqTrans(int) const { return false; }

    void		setDisplayTransformation(mVisTrans*);
    mVisTrans*		getDisplayTransformation();
    void		setRightHandSystem(bool);


    bool		setVisSurface(visBase::MarchingCubesSurface*);
    			//!<Creates an EMObject for it.
    bool		setEMID(const EM::ObjectID&);
    EM::ObjectID	getEMID() const;

    void		showManipulator(bool);
    bool		isManipulatorShown() const;

    bool		hasInitialShape();
    bool		createInitialBody(bool allowswap);
    void		removeInitialDragger();

protected:

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    virtual			~MarchingCubesDisplay();
    void			updateVisFromEM(bool onlyshape);
    void			initialDraggerMovedCB(CallBacker*);
    void			initialDraggerMovingCB(CallBacker*);
    void			kernelDraggerMovedCB(CallBacker*);
    void			kernelDraggerMovingCB(CallBacker*);
    virtual void		fillPar(IOPar&,TypeSet<int>& saveids) const;
    virtual int			usePar(const IOPar&);
    void			factorDrag(CallBacker*);
    void			setDragDirection(CallBacker*);
    void			setNormalLine(Coord3& center,Coord3& width);
    Array3D<unsigned char>*	createKernel(int xsz, int ysz, int zsz ) const;

    visBase::BoxDragger*		initialdragger_;
    visBase::MarchingCubesSurface*	displaysurface_;
    EM::MarchingCubesSurface*		emsurface_;
    MarchingCubesSurfaceEditor*		surfaceeditor_;
    visBase::InvisibleLineDragger*	factordragger_;
    bool				allowdrag_;
    visBase::EventCatcher*		eventcatcher_;
    visBase::Ellipsoid*			initialellipsoid_;
    visBase::PickStyle*			kernelpickstyle_;
    visBase::Ellipsoid*			kernelellipsoid_;
    visBase::Dragger*			kerneldragger_;
    visBase::IndexedPolyLine*		normalline_;

    double				minsampleinlsz_;
    double				minsamplecrlsz_;
    double				minsamplezsz_;
    Coord3				kernelsize_;
    CubeSampling			previoussample_;
};

};


#endif
