#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:    N. Hemstra
Date:        August 2002
________________________________________________________________________


-*/

#include "mousecursor.h"
#include "vissurveymod.h"
#include "vissurvobj.h"
#include "visobject.h"

namespace Attrib { class SelSpec; }
template <class T> class Selector;
class RegularSeisDataPack;
class ZAxisTransform;

namespace visBase
{
    class BoxDragger;
    class TextureChannels;
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

mExpClass(visSurvey) MPEDisplay : public visBase::VisualObjectImpl,
		    public visSurvey::SurveyObject
{
public:
		    MPEDisplay();
		    mDefaultFactoryInstantiation(
			visSurvey::SurveyObject,MPEDisplay,
			     "MPEDisplay", ::toUiString(sFactoryKeyword()));

    bool            isInlCrl() const	{ return true; }
    bool            isOn() const;

    void            showBoxDragger(bool);
    bool            isBoxDraggerShown() const;

    void            showDragger(bool yn);
    bool            isDraggerShown() const;

    void	    enablePicking(bool);
    bool	    isPickingEnabled() const;

    void            setDraggerTransparency(float);
    float           getDraggerTransparency() const;

    void            setPlaneOrientation(int orient);
    int		    getPlaneOrientation() const;

    bool	    getPlanePosition(TrcKeyZSampling&) const;
    void            moveMPEPlane(int nrsteps);

    void            updateBoxSpace();
    void            freezeBoxPosition(bool yn);

    TrcKeyZSampling	getTrcKeyZSampling(int attrib=-1) const;

    void            setSelSpec(int,const Attrib::SelSpec&);
    const char*		getSelSpecUserRef() const;
                    /*!<\returns the userRef, "None" if
                     selspec.id==NoAttrib, or a zeropointer
                     if selspec.id==notsel */
    const Attrib::SelSpec*	getSelSpec(int attrib,int version=0) const;

    const ColTab::MapperSetup*  getColTabMapperSetup(int, int version=0) const;
    void            setColTabMapperSetup(int,
                    const ColTab::MapperSetup&,TaskRunner*);

    const ColTab::Sequence*    getColTabSequence(int) const;
    bool            canSetColTabSequence() const;
    void            setColTabSequence(int,const ColTab::Sequence&,
	                              TaskRunner*);

    const MouseCursor*	 getMouseCursor() const { return &mousecursor_; }

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

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar( const IOPar&);

    NotifierAccess*	getMovementNotifier();

    Notifier<MPEDisplay>	boxDraggerStatusChange;
    Notifier<MPEDisplay>	planeOrientationChange;

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

    void		setRightHandSystem(bool yn);

    bool		setDataVolume(int attrib,const RegularSeisDataPack*,
				      TaskRunner*);
    void		setTrcKeyZSampling(const TrcKeyZSampling&);

    const RegularSeisDataPack*	getCacheVolume(int attrib) const;
    bool		setDataPackID(int attrib,DataPack::ID,TaskRunner*);
    DataPack::ID	getDataPackID(int attrib) const;
    virtual DataPackMgr::MgrID	getDataPackMgrID() const
                                { return DataPackMgr::SeisID(); }

    virtual bool        allowsPicks() const;
    void		allowShading(bool yn );
    bool		hasPosModeManipulator() const		{ return true; }
    void		showManipulator(bool yn);
    bool		isManipulated() const;
    bool		canResetManipulation() const;
    void		resetManipulation();
    void		acceptManipulation();
    BufferString	getManipulationString() const;
    NotifierAccess*	getManipulationNotifier();

    static int		cInLine()	{ return 0; }
    static int		cCrossLine()	{ return 1; }
    static int		cTimeSlice()	{ return 2; }

    // texture channel-related methods

    SurveyObject::AttribFormat	getAttributeFormat(int attrib=-1) const;

    bool		canAddAttrib(int nrattribstoadd=1) const;
    bool                canRemoveAttrib() const;
    int                 nrAttribs() const;
    bool                addAttrib();
    bool                removeAttrib(int attrib);
    void		enableAttrib(int attrib,bool yn);
    bool		isAttribEnabled(int attrib) const;

    const char*		errMsg() const { return errmsg_.str(); }

    void		setDisplayTransformation(const mVisTrans*);


protected:
			~MPEDisplay();
    TrcKeyZSampling	getBoxPosition() const;
    void		setPlanePosition(const TrcKeyZSampling&);

    void		setSliceDimension(int slice,int dim);
    void		alignSliceToSurvey(visBase::OrthogonalSlice&);

    void		setSceneEventCatcher(visBase::EventCatcher*);

    // callback from boxdragger
    void		boxDraggerFinishCB(CallBacker*);

    // callbacks from MPE
    void		updateBoxPosition(CallBacker*);

    // methods for volume-based display
    TrcKeyZSampling	getTrcKeyZSampling(bool manippos,bool display,
					int attrib) const;

    void		triggerSel();
    void		triggerDeSel();

    bool		pickable() const	{ return true; }
    bool		rightClickable() const	{ return false; }
    bool		selectable() const	{ return false; }  // check!
    bool		isSelected() const;

    void		turnOnSlice(bool);
    void		updateRanges(bool updateic,bool updatez);

    // callback from user
    void		mouseClickCB(CallBacker*);
    void		updateMouseCursorCB(CallBacker*);

    // other callbacks
    void		dataTransformCB(CallBacker*);
    void		sliceMoving(CallBacker*);

    // texture channel-related methods
    bool		updateFromCacheID(int attrib, TaskRunner* tr);

    MPE::Engine&		engine_;
    visBase::BoxDragger*	boxdragger_;
    visBase::EventCatcher*	sceneeventcatcher_;
    MouseCursor			mousecursor_;
    Notifier<MPEDisplay>	movement;
    Attrib::SelSpec&		as_;
    bool			manipulated_;
    int				lasteventnr_;

    Attrib::SelSpec&		curtextureas_;
    TrcKeyZSampling		curtexturecs_;

    // data for volume-based display
    visBase::Transformation*	voltrans_;
    ObjectSet<visBase::OrthogonalSlice>	slices_;
    DataPack::ID		cacheid_;
    const RegularSeisDataPack*  volumecache_;
    BufferString		sliceposition_;
    BufferString		slicename_;
    TrcKeyZSampling		csfromsession_;
    bool			issliceshown_;
    bool			allowshading_;
    int				dim_;
    ZAxisTransform*		datatransform_;

    // texture channel-related data
    visBase::TextureChannels*	channels_;

    ConstRefMan<mVisTrans>	displaytrans_;
    Coord3			curboxcenter_;
    Coord3			curboxwidth_;

    // common keys
    static const char*		sKeyTransparency() { return "Transparency"; }
    static const char*		sKeyBoxShown()     { return "Box Shown"; }

    static const char*		sKeyBoxDepthKey()   { return "BoxDepthKey"; }
    static const char*		sKeyBoxPlaneKey()   { return "BoxPlaneKey"; }
    static const char*		sKeyInDepthBoxResize()
						{ return "InDepthBoxResize"; }

    static const OD::Color	reTrackColor;
    static const OD::Color	eraseColor;
    static const OD::Color	movingColor;
    static const OD::Color	extendColor;

    // volume-related keys
    static const char*		sKeyNrSlices()	{ return "Nr of slices"; }
    static const char*		sKeySlice()	{ return "SliceID"; }
    static const char*		sKeyInline()	{ return "Inline"; }
    static const char*		sKeyCrossLine()	{ return "Crossline"; }
    static const char*		sKeyTime()	{ return "Time"; }

};

}; // namespace visSurvey

