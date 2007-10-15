#ifndef vismarchingcubessurfacedisplay_h
#define vismarchingcubessurfacedisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismarchingcubessurfacedisplay.h,v 1.8 2007-10-15 22:27:53 cvsyuancheng Exp $
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
    class MarchingCubesSurface;
    class PickStyle;
    class Transformation;
};


namespace EM { class MarchingCubesSurface; }


namespace visSurvey
{
class Scene;

/*!\brief Used for displaying welltracks, markers and logs


*/

class MarchingCubesDisplay : public visBase::VisualObjectImpl,
			     public visSurvey::SurveyObject
{
public:
    static MarchingCubesDisplay*create()
				mCreateDataObj(MarchingCubesDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return true; }

    bool			hasColor() const	{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const { return true; }
    NotifierAccess*		materialChange();

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    void			setRightHandSystem(bool);

    void			setSceneEventCatcher(visBase::EventCatcher*);

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    bool			setVisSurface(visBase::MarchingCubesSurface*);
    				//!<Creates an EMObject for it.
    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

    void			showManipulator(bool);
    bool			isManipulatorShown() const;

    bool			hasInitialShape();
    bool			createInitialBody(bool allowswap);
    void			removeInitialDragger();

protected:

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    virtual			~MarchingCubesDisplay();
    void			updateVisFromEM(bool onlyshape);
    void			draggerMovedCB(CallBacker*);
    void			draggerMovingCB(CallBacker*);
    void			pickCB(CallBacker*);
    void			factorDrag(CallBacker*);
    Array3D<unsigned char>*	createKernel(int xsz, int ysz, int zsz ) const;

    float				startpos_;
    visBase::BoxDragger*		initialdragger_;
    visBase::MarchingCubesSurface*	displaysurface_;
    EM::MarchingCubesSurface*		emsurface_;
    MarchingCubesSurfaceEditor*		surfaceeditor_;
    visBase::Dragger*			factordragger_;
    visBase::EventCatcher*		eventcatcher_;
    visBase::Ellipsoid*			initialellipsoid_;
    visBase::PickStyle*			kernelpickstyle_;
    visBase::Ellipsoid*			kernelellipsoid_;

    double				minsampleinlsz_;
    double				minsamplecrlsz_;
    double				minsamplezsz_;
    CubeSampling			previoussample_;
};

};


#endif
