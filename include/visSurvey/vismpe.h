#ifndef vismpe_h
#define vismpe_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: vismpe.h,v 1.38 2009-05-07 07:32:58 cvsumesh Exp $
________________________________________________________________________


-*/

#include "vissurvobj.h"
#include "visobject.h"
#include "cubesampling.h"

namespace Attrib { class SelSpec; }
template <class T> class Array3D;
template <class T> class Selector;

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

mClass MPEDisplay : public visBase::VisualObjectImpl,
		   public visSurvey::SurveyObject
{
public:

    static MPEDisplay*		create()
				mCreateDataObj(MPEDisplay);
    bool			isInlCrl() const	{ return true; }

    bool			isOn() const;
    void			updateSeedOnlyPropagation(bool);
    void			updateMPEActiveVolume();
    void			removeSelectionInPolygon(
	    					const Selector<Coord3>&);
	    
    void			showBoxDragger(bool);
    bool			isBoxDraggerShown() const;
    void			setDraggerTransparency(float);
    float			getDraggerTransparency() const;
    void			showDragger(bool yn);
    void			setPlaneOrientation(int orient);
    bool			isDraggerShown() const;
    void			moveMPEPlane(int nrsteps);
    visBase::Texture3*		getTexture() { return texture_; }

    CubeSampling		getCubeSampling(int attrib=-1) const;
    bool			getPlanePosition(CubeSampling&) const;

    void			setSelSpec(int,const Attrib::SelSpec&);
    const char*			getSelSpecUserRef() const;
    				/*!<\returns the userRef, "None" if
				     selspec.id==NoAttrib, or a zeropointer 
				     if selspec.id==notsel */ 
    void			updateTexture();
    void			updateBoxSpace();
    void			freezeBoxPosition(bool yn);

    NotifierAccess*		getMovementNotifier() { return &movement; }
    Notifier<MPEDisplay>	boxDraggerStatusChange;
    
    virtual float               calcDist(const Coord3&) const;
    virtual float               maxDist() const;

    void			getMousePosInfo(const visBase::EventInfo&,
	    					const Coord3&,
						BufferString& val,
						BufferString& info) const;

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

    Attrib::SelSpec&		curtextureas_;
    CubeSampling		curtexturecs_;
    
    int				lasteventnr_;

    static const char*		sKeyTransperancy() { return "Transparency"; }
    static const char*		sKeyTexture()      { return "Texture"; }
    static const char*		sKeyBoxShown()     { return "Box Shown"; }

    static const Color		reTrackColor;
    static const Color		eraseColor;
    static const Color		movingColor;
    static const Color		extendColor;
};

};


#endif
