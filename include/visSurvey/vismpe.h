#ifndef vismpe_h
#define vismpe_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: vismpe.h,v 1.25 2005-10-11 22:16:25 cvskris Exp $
________________________________________________________________________


-*/

#include "vissurvobj.h"
#include "visobject.h"
#include "cubesampling.h"

namespace Attrib { class SelSpec; }
template <class T> class Array3D;

namespace visBase
{
    class BoxDragger;
    class DataObjectGroup;
    class DepthTabPlaneDragger;
    class FaceSet;
    class Texture3;
    class Transformation;
};

namespace MPE { class Engine; };


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

    bool			isOn() const;
    bool			isManipulated() const	{ return manipulated_; }
    void			acceptManipulation()	{ manipulated_=false; }
    void			updateMPEActiveVolume();
    void			showManipulator(bool);
    bool			isManipulatorShown() const;
    void			setDraggerTransparency(float);
    float			getDraggerTransparency() const;
    void			showDragger(bool);
    bool			isDraggerShown() const;
    void			moveMPEPlane(int nrsteps);
    void			updatePlaneColor();
    visBase::Texture3*		getTexture() { return texture_; }

    CubeSampling		getCubeSampling() const;
    void			setCubeSampling(CubeSampling);
    bool			getPlanePosition(CubeSampling&) const;

    void			setSelSpec(const Attrib::SelSpec&);
    const char*			getSelSpecUserRef() const;
    				/*!<\returns the userRef, "None" if
				     selspec.id==NoAttrib, or a zeropointer 
				     if selspec.id==notsel */ 
    void			updateTexture();

    NotifierAccess*		getMovementNotification() { return  &movement; }
    
    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar( const IOPar&);

protected:
				~MPEDisplay();
    CubeSampling		getBoxPosition() const;

    void			setPlanePosition(const CubeSampling&);

    void			setTexture(visBase::Texture3*);
    void			updateTextureCoords();
    void			setDraggerCenter(bool alldims);
    void			setDragger(visBase::DepthTabPlaneDragger*);

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

    MPE::Engine&		engine_;

    visBase::DataObjectGroup*	draggerrect_;
    visBase::FaceSet*		rectangle_;
    visBase::DepthTabPlaneDragger* dragger_;
    visBase::BoxDragger*	boxdragger_;
    visBase::Texture3*		texture_;

    visBase::EventCatcher*	sceneeventcatcher_;

    Notifier<MPEDisplay>	movement;

    Attrib::SelSpec&		as_;
    bool			manipulated_;
    
    int				lasteventnr_;

    static const char*		sKeyTransperancy() { return "Transparency"; }
    static const char*		sKeyTexture()      { return "Texture"; }

    static const Color		reTrackColor;
    static const Color		eraseColor;
    static const Color		movingColor;
    static const Color		extendColor;
};

};


#endif
