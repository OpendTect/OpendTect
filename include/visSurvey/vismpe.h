#ifndef vismpe_h
#define vismpe_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: vismpe.h,v 1.4 2005-03-09 16:44:17 cvsnanne Exp $
________________________________________________________________________


-*/

#include "cubesampling.h"
#include "visobject.h"
#include "vissurvobj.h"

class AttribSelSpec;
template <class T> class Array3D;

namespace visBase
{
    class BoxDragger;
    class DepthTabPlaneDragger;
    class FaceSet;
    class Transformation;
    class DataObjectGroup;
};

namespace MPE { class Plane; class Engine; };


namespace visSurvey
{

/*!\brief

*/

class MPEDisplay : public visBase::VisualObjectImpl,
		   public visSurvey::SurveyObject
{
public:

    static MPEDisplay*		create()
				mCreateDataObj(MPEDisplay);
    bool			isInlCrl() const	{ return true; }

    enum TrackMode		{ Extend, ReTrack, Erode };
    TrackMode			getTrackMode() const	{ return trackmode_; }
    void			setTrackMode(const TrackMode& tm)
				{ trackmode_ = tm; }

    void			showManipulator(bool);
    void			setDraggerTransparency(float);
    float			getDraggerTransparency() const;
    void			showDragger(bool);
    bool			isDraggerShown() const;
    void			moveMPEPlane(int nrsteps);
    
    CubeSampling		getCubeSampling() const;
    void			setCubeSampling(CubeSampling);

    void			setSelSpec(const AttribSelSpec&);
    const AttribSelSpec*	getSelSpec() const;
    void			updateTexture();

protected:
				~MPEDisplay();
    CubeSampling		getBoxPosition() const;

    bool			getPlanePosition(CubeSampling&) const;
    void			setPlanePosition(const CubeSampling&);

    void			setSceneEventCatcher(visBase::EventCatcher*);

				//Callbacks from boxdragger
    void			boxDraggerFinishCB(CallBacker*);

    				//Callbacks from rectangle
    void			rectangleMovedCB(CallBacker*);
    void			rectangleStartCB(CallBacker*);
    void			rectangleStopCB(CallBacker*);

    				//Callbacks from user
    void			mouseClickCB(CallBacker*);

    				//Callbacks from MPE
    void			updateDraggerPosition(CallBacker*);
    void			updateBoxPosition(CallBacker*);

    MPE::Engine&		engine;

    visBase::DataObjectGroup*	draggerrect_;
    visBase::FaceSet*		rectangle_;
    visBase::DepthTabPlaneDragger* dragger_;
    visBase::BoxDragger*	boxdragger_;

    visBase::EventCatcher*	sceneeventcatcher_;

    TrackMode			trackmode_;
    AttribSelSpec&		as_;
};

};


#endif
