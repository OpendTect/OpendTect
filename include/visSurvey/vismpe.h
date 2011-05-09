#ifndef vismpe_h
#define vismpe_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:    N. Hemstra
Date:        August 2002
RCS:        $Id: vismpe.h,v 1.68 2011-05-09 08:48:09 cvsbert Exp $
________________________________________________________________________


-*/

#include "vissurvobj.h"
#include "mousecursor.h"
#include "visobject.h"
#include "cubesampling.h"
#include "attribdatapack.h"

namespace Attrib { class SelSpec; class CubeDataPack; }
template <class T> class Array3D;
template <class T> class Selector;
class ZAxisTransform;

namespace visBase
{
    class BoxDragger;
    class DataObjectGroup;
    class DepthTabPlaneDragger;
    class FaceSet;
    class Texture3;
    class TextureChannels;
    class TextureChannel2VolData;
    class OrthogonalSlice;
    class Transformation;
};

namespace ColTab { class MapperSetup; class Sequence; }
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

    // common methods (to both texture-based and volume-based display)
    static MPEDisplay*        create()
	    	    mCreateDataObj(MPEDisplay);
  
    bool            isInlCrl() const	{ return true; }
    bool            isOn() const;

    void            showBoxDragger(bool);
    bool            isBoxDraggerShown() const;
    
    void            showDragger(bool yn);
    bool            isDraggerShown() const;
    
    void            setDraggerTransparency(float);
    float           getDraggerTransparency() const;
    
    void            setPlaneOrientation(int orient);
    const int       getPlaneOrientation() const;

    bool            getPlanePosition(CubeSampling&) const;
    void            moveMPEPlane(int nrsteps);    

    void            updateBoxSpace();
    void            freezeBoxPosition(bool yn);

    CubeSampling	getCubeSampling(int attrib=-1) const;
   
    void            setSelSpec(int,const Attrib::SelSpec&);
    const char*		getSelSpecUserRef() const;
                    /*!<\returns the userRef, "None" if
                     selspec.id==NoAttrib, or a zeropointer
                     if selspec.id==notsel */
    const Attrib::SelSpec*	getSelSpec(int attrib) const;

    const ColTab::MapperSetup*  getColTabMapperSetup(int, int version=0) const;
    void            setColTabMapperSetup(int,
                    const ColTab::MapperSetup&,TaskRunner*);
    
    const ColTab::Sequence*    getColTabSequence(int) const;
    bool            canSetColTabSequence() const;
    void            setColTabSequence(int,const ColTab::Sequence&,
	                              TaskRunner*);

    void	    getMousePosInfo( const visBase::EventInfo& ei,
				     IOPar& iop ) const
		    { return SurveyObject::getMousePosInfo(ei,iop);}
    void            getMousePosInfo(const visBase::EventInfo&, Coord3&, 
	                          BufferString& val, BufferString& info) const;
    void	    getObjectInfo(BufferString&) const;

    void            updateSeedOnlyPropagation(bool);
    void            updateMPEActiveVolume();
    void            removeSelectionInPolygon(const Selector<Coord3>&,
	    				     TaskRunner*);

    virtual float	calcDist(const Coord3&) const;
    virtual float       maxDist() const;

    virtual void	fillPar(IOPar&,TypeSet<int>&) const;
    virtual int		usePar( const IOPar&);

    NotifierAccess*	getMovementNotifier();
    
    Notifier<MPEDisplay>	boxDraggerStatusChange;
    Notifier<MPEDisplay>	planeOrientationChange;
	

    // methods for texture-based display
    visBase::Texture3*	getTexture() { return texture_; }
    void		updateTexture();
    

    // methods for volume-based display
    int			addSlice(int dim, bool show);
    visBase::OrthogonalSlice*	getSlice(int index);
    void		updateSlice();
    float		slicePosition(visBase::OrthogonalSlice*) const;
    float		getValue(const Coord3&) const;   
	
    void		removeChild(int displayid);
    void		getChildren(TypeSet<int>&) const;
	
    bool		setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;
   
    bool		setDataVolume(int attrib,const Attrib::CubeDataPack*,
	    				TaskRunner*);
    virtual bool	setDataVolume( int attr, const Attrib::DataCubes* dc,
				      TaskRunner* tr )
			{ return SurveyObject::setDataVolume(attr,dc,tr); }
    void		setCubeSampling(const CubeSampling&);
    
    const Attrib::DataCubes*	getCacheVolume(int attrib) const;
    bool		setDataPackID(int attrib,DataPack::ID,TaskRunner*);
    DataPack::ID	getDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const
                                    { return DataPackMgr::CubeID(); }
    
    virtual bool        allowsPicks() const;
    void		allowShading(bool yn );
    void		showManipulator(bool yn);
    bool		isManipulated() const;
    bool		canResetManipulation() const;
    void		resetManipulation();
    void		acceptManipulation();
    BufferString	getManipulationString() const;
    NotifierAccess*	getManipulationNotifier();

    static int		cInLine()	{ return 2; }
    static int		cCrossLine()	{ return 1; }
    static int		cTimeSlice()	{ return 0; }
   

    // new methods for texture channel-based display
    void		clearTextures();
    
    void		setChannels2VolData(visBase::TextureChannel2VolData*);
    visBase::TextureChannel2VolData*	getChannels2VolData();

    SurveyObject::AttribFormat	getAttributeFormat(int attrib=-1) const;

    bool		canAddAttrib(int nrattribstoadd=1) const;
    bool                canRemoveAttrib() const;
    int                 nrAttribs() const;
    bool                addAttrib();
    bool                removeAttrib(int attrib);
    void		enableAttrib(int attrib,bool yn);
    bool		isAttribEnabled(int attrib) const;

/*    void                setAttribTransparency(int,unsigned char);
    unsigned char       getAttribTransparency(int) const;*/

    const char*		errMsg() const { return errmsg_.str(); }
    
protected:
    
    // common methods (to both texture-based and volume-based display)
    			~MPEDisplay();
    CubeSampling	getBoxPosition() const;
    void		setPlanePosition(const CubeSampling&);
	
    void		setSceneEventCatcher(visBase::EventCatcher*);
	
			//Callback from boxdragger
    void		boxDraggerFinishCB(CallBacker*);

			//Callbacks from MPE
    void		updateDraggerPosition(CallBacker*);
    void		updateBoxPosition(CallBacker*);


    // methods for texture-based display
    void		setTexture(visBase::Texture3*);
    void		updateTextureCoords();
    
    void		setDraggerCenter(bool alldims);
    void		setDragger(visBase::DepthTabPlaneDragger*);

			//Callbacks from rectangle
    void		rectangleMovedCB(CallBacker*);
    void		rectangleStartCB(CallBacker*);
    void		rectangleStopCB(CallBacker*);


    // methods for volume-based display
    CubeSampling	getCubeSampling(bool manippos,bool display,
	    		    		int attrib) const;
    void		updateFromData(const Array3D<float>* arr,
	    			       bool arrayismine,TaskRunner*);

    const MouseCursor*	getMouseCursor() const	{ return &mousecursor_; }

    void		triggerSel();
    void		triggerDeSel();

    bool		pickable() const	{ return true; }
    bool		rightClickable() const	{ return false; }
    bool		selectable() const	{ return false; }  // check!
    bool		isSelected() const;
   
    void		turnOnSlice(bool);
    void		updateRanges(bool updateic,bool updatez);
   
			// Callbacks from user
    void		mouseClickCB(CallBacker*);
    void		updateMouseCursorCB(CallBacker*);

    			// Other callbacks
    void		dataTransformCB(CallBacker*);
    void		sliceMoving(CallBacker*);
    

    // new methods for texture channel-based display
    void		updateFromDataPackID(int attrib, const DataPack::ID 
	    				     newdpid, TaskRunner* tr);
    void		updateFromCacheID(int attrib, TaskRunner* tr);


    // --------------
    // common data    
    MPE::Engine&		engine_;
    visBase::BoxDragger*	boxdragger_;
    visBase::EventCatcher*	sceneeventcatcher_;    
    Notifier<MPEDisplay>	movement;    
    Attrib::SelSpec&		as_;
    bool			manipulated_;
    int				lasteventnr_;

    
    // data for texture-based display
    visBase::DataObjectGroup*	draggerrect_;
    visBase::FaceSet*		rectangle_;
    visBase::DepthTabPlaneDragger*	dragger_;    
    visBase::Texture3*		texture_;
    Attrib::SelSpec&		curtextureas_;
    CubeSampling		curtexturecs_;
    
    
    // data for volume-based display
    visBase::Transformation*	voltrans_;
    ObjectSet<visBase::OrthogonalSlice>	slices_;
    MouseCursor			mousecursor_;
//    Notifier<MPEDisplay>	slicemoving;
    DataPack::ID		cacheid_;
    const Attrib::CubeDataPack* volumecache_;
    BufferString		sliceposition_;
    BufferString		slicename_;
    CubeSampling		csfromsession_;
    bool			isinited_;
    bool			allowshading_;
    int				dim_;
    ZAxisTransform*		datatransform_;
   

    // data for new texture channel-based display
    visBase::TextureChannels*	channels_;
    

    // --------------
    // common keys
    static const char*		sKeyTransparency() { return "Transparency"; }
    static const char*		sKeyBoxShown()     { return "Box Shown"; }
    static const Color		reTrackColor;
    static const Color		eraseColor;
    static const Color		movingColor;
    static const Color		extendColor;


    // texture-related keys
    static const char*		sKeyTexture()      { return "Texture"; }
	

    // volume-related keys
    static const char*		sKeyNrSlices()	{ return "Nr of slices"; }
    static const char*		sKeySlice()	{ return "SliceID"; }
    static const char*		sKeyInline()	{ return "Inline"; }
    static const char*		sKeyCrossLine()	{ return "Crossline"; }
    static const char*		sKeyTime()	{ return "Time"; }


    // new texture channel-related keys    
    static const char*		sKeyTC2VolData()	{ return "TC2VolData"; }

    virtual SoNode*		gtInvntrNode();

};

}; // namespace visSurvey

#endif

