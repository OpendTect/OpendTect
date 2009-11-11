#ifndef vismpe_h
#define vismpe_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: vismpe.h,v 1.51 2009-11-11 16:43:51 cvskarthika Exp $
________________________________________________________________________


-*/

#include "vissurvobj.h"
#include "mousecursor.h"
#include "visobject.h"
#include "cubesampling.h"

namespace Attrib { class SelSpec; class DataCubes; }
template <class T> class Array3D;
template <class T> class Selector;
class ZAxisTransform;
class ZAxisTransformer;


class SoSeparator;

namespace visBase
{
    class BoxDragger;
    class DataObjectGroup;
    class DepthTabPlaneDragger;
    class FaceSet;
    class Texture3;
    class VolumeRenderScalarField;
    class OrthogonalSlice;
    class Transformation;
};

namespace ColTab { struct MapperSetup; class Sequence; }
namespace MPE { class Engine; };
class TaskRunner;


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
    
    static int			cInLine() 		{ return 2; }
    static int			cCrossLine() 		{ return 1; }
    static int			cTimeSlice() 		{ return 0; }
    
    bool			isInlCrl() const	{ return true; }
    bool			isOn() const;
    void			updateSeedOnlyPropagation(bool);
    void			updateMPEActiveVolume();
    void			removeSelectionInPolygon(
	    					const Selector<Coord3>&,
						TaskRunner*);
	    
    void			showBoxDragger(bool);
    bool			isBoxDraggerShown() const;
    void			setDraggerTransparency(float);
    float			getDraggerTransparency() const;
    void			showDragger(bool yn);
    void			setPlaneOrientation(int orient);
    const int			getPlaneOrientation() const;
    bool			isDraggerShown() const;
    void			moveMPEPlane(int nrsteps);
    visBase::Texture3*		getTexture() { return texture_; }
    visBase::OrthogonalSlice*	getSlice(int index);
    float                       slicePosition(visBase::OrthogonalSlice*) const;
    float                       getValue(const Coord3&) const;    

    const ColTab::MapperSetup*  getColTabMapperSetup(int) const;
    void			setColTabMapperSetup(int,
					const ColTab::MapperSetup&,TaskRunner*);
    const ColTab::Sequence*	getColTabSequence(int) const;
    bool			canSetColTabSequence() const;
    void			setColTabSequence(int,const ColTab::Sequence&,
	    					  TaskRunner*);

    int				addSlice(int dim, bool show);
    void                        removeChild(int displayid);
    void			getChildren(TypeSet<int>&) const;

    CubeSampling		getCubeSampling(int attrib=-1) const;
    void			setCubeSampling(const CubeSampling&);
    bool			getPlanePosition(CubeSampling&) const;
 
    bool                        setDataTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*       getDataTransform() const;
    
    bool                        setDataVolume(int attrib, 
	    				const Attrib::DataCubes*, TaskRunner*);
    const Attrib::DataCubes*	getCacheVolume(int attrib) const;
    bool			setDataPackID(int attrib,DataPack::ID,
	    				      TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    virtual DataPackMgr::ID     getDataPackMgrID() const
	                                { return DataPackMgr::CubeID(); }
    
    virtual bool		allowPicks() const;

    void                        showManipulator(bool yn);
    bool                        isManipulated() const;
    bool                        canResetManipulation() const;
    void                        resetManipulation();
    void			acceptManipulation();
    BufferString                getManipulationString() const;

    void			setSelSpec(int,const Attrib::SelSpec&);
    const char*			getSelSpecUserRef() const;
    				/*!<\returns the userRef, "None" if
				     selspec.id==NoAttrib, or a zeropointer 
				     if selspec.id==notsel */ 
    const Attrib::SelSpec*	getSelSpec(int) const
    				{ return &as_; }
    void			updateTexture();
    void			updateSlice();
    void			updateBoxSpace();
    void			freezeBoxPosition(bool yn);

    NotifierAccess*		getMovementNotifier();
    NotifierAccess*             getManipulationNotifier();
    Notifier<MPEDisplay>	boxDraggerStatusChange;
    Notifier<MPEDisplay>	planeOrientationChange;
    
    virtual float               calcDist(const Coord3&) const;
    virtual float               maxDist() const;

    void			getMousePosInfo(const visBase::EventInfo&,
	    					Coord3&,
						BufferString& val,
						BufferString& info) const;

    void			allowShading(bool yn ) { allowshading_ = yn; }

    SoNode*			getInventorNode();

    virtual void		fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar( const IOPar&);
    
protected:
				~MPEDisplay();
    CubeSampling		getBoxPosition() const;

    void			setPlanePosition(const CubeSampling&);

    void			setTexture(visBase::Texture3*);
    void			updateTextureCoords();
    void			updateSliceCoords();
    
    void			setDraggerCenter(bool alldims);
    void			setDragger(visBase::DepthTabPlaneDragger*);

    void			setSceneEventCatcher(visBase::EventCatcher*);

    CubeSampling		getCubeSampling(bool manippos,bool display,
		    					int attrib) const;

    bool			pickable() const { return true; }
    bool			rightClickable() const { return false; }
    bool			selectable() const { return false; }  // check
    bool			isSelected() const;
    
    void			turnOnSlice(bool);

    const MouseCursor*		getMouseCursor() const { return &mousecursor_; }

    void			triggerSel() { updateMouseCursorCB( 0 ); }
    void			triggerDeSel() { updateMouseCursorCB( 0 ); }

				//Callbacks from boxdragger
    void			boxDraggerFinishCB(CallBacker*);

    				//Callbacks from rectangle
    void			rectangleMovedCB(CallBacker*);
    void			rectangleStartCB(CallBacker*);
    void			rectangleStopCB(CallBacker*);

    				//Callbacks from user
    void			mouseClickCB(CallBacker*);
    void			updateMouseCursorCB(CallBacker*);
    
    				//Callbacks from MPE
    void			updateDraggerPosition(CallBacker*);
    void			updateBoxPosition(CallBacker*);

    void                        sliceMoving(CallBacker*);
    void                        dataTransformCB(CallBacker*);
    void                        updateRanges(bool updateic,bool updatez);
    
    MPE::Engine&		engine_;

    visBase::DataObjectGroup*	draggerrect_;
    visBase::FaceSet*		rectangle_;
    visBase::DepthTabPlaneDragger* dragger_;
    visBase::BoxDragger*	boxdragger_;

    visBase::Texture3*		texture_;
    visBase::VolumeRenderScalarField*	scalarfield_;
    visBase::Transformation*		voltrans_;
    ObjectSet<visBase::OrthogonalSlice>	slices_;

    visBase::EventCatcher*	sceneeventcatcher_;
    MouseCursor			mousecursor_;

    Notifier<MPEDisplay>	movement;
    Notifier<MPEDisplay>        slicemoving;

    Attrib::SelSpec&		as_;
    bool			manipulated_;

    Attrib::SelSpec&		curtextureas_;
    CubeSampling		curtexturecs_;
   
    DataPack::ID                cacheid_;
    const Attrib::DataCubes*    cache_;
    BufferString                sliceposition_;
    BufferString                slicename_;
    CubeSampling                csfromsession_;
    int				dim_;

    ZAxisTransform*             datatransform_;
    ZAxisTransformer*           datatransformer_;
    
    bool			allowshading_;
    visBase::EventCatcher*	eventcatcher_;

    bool			isinited_;

    int				lasteventnr_;

    static const char*		sKeyTransperancy() { return "Transparency"; }
    static const char*		sKeyTexture()      { return "Texture"; }
    static const char*		sKeyNrSlices() { return "Nr of slices"; }
    static const char*		sKeySlice() { return "SliceID"; }
    static const char*		sKeyBoxShown()     { return "Box Shown"; }
    static const char*		sKeyInline()		{ return "Inline"; } 
    static const char*		sKeyCrossLine()	{ return "Crossline"; }
    static const char*		sKeyTime()		{ return "Time"; }

    static const Color		reTrackColor;
    static const Color		eraseColor;
    static const Color		movingColor;
    static const Color		extendColor;
};

}; // namespace visSurvey


#endif
