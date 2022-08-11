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

mExpClass(visSurvey) MPEDisplay : public visBase::VisualObjectImpl
				, public visSurvey::SurveyObject
{
public:
			MPEDisplay();
			mDefaultFactoryInstantiation(
			    visSurvey::SurveyObject,MPEDisplay,
				 "MPEDisplay", ::toUiString(sFactoryKeyword()));

    bool		isInlCrl() const override	{ return true; }
    bool		isOn() const override;

    void		showBoxDragger(bool);
    bool		isBoxDraggerShown() const;

    void		showDragger(bool yn);
    bool		isDraggerShown() const;

    void		enablePicking(bool);
    bool		isPickingEnabled() const;

    void		setDraggerTransparency(float);
    float		getDraggerTransparency() const;

    void		setPlaneOrientation(int orient);
    int			getPlaneOrientation() const;

    bool		getPlanePosition(TrcKeyZSampling&) const;
    void		moveMPEPlane(int nrsteps);

    void		updateBoxSpace();
    void		freezeBoxPosition(bool yn);

    TrcKeyZSampling	getTrcKeyZSampling(int attrib=-1) const override;

    void		setSelSpec(int,const Attrib::SelSpec&) override;
    const char*		getSelSpecUserRef() const;
			/*!<\returns the userRef, "None" if
			selspec.id==NoAttrib, or a zeropointer
			if selspec.id==notsel */
    const Attrib::SelSpec*	getSelSpec(int attrib,
					   int version=0) const override;

    const ColTab::MapperSetup*  getColTabMapperSetup(int,
						int version=0) const override;
    void		setColTabMapperSetup(int,const ColTab::MapperSetup&,
					 TaskRunner*) override;

    const ColTab::Sequence* getColTabSequence(int) const override;
    bool		canSetColTabSequence() const override;
    void		setColTabSequence(int,const ColTab::Sequence&,
					  TaskRunner*) override;

    const MouseCursor*	 getMouseCursor() const override
			{ return &mousecursor_; }

    void		getMousePosInfo( const visBase::EventInfo& ei,
					 IOPar& iop ) const override
			{ return SurveyObject::getMousePosInfo(ei,iop); }
    void		getMousePosInfo(const visBase::EventInfo&,
					Coord3&,BufferString& val,
					BufferString& info) const override;
    void		getObjectInfo(BufferString&) const override;

    void		updateSeedOnlyPropagation(bool);
    void		updateMPEActiveVolume();
    void		removeSelectionInPolygon(const Selector<Coord3>&,
					     TaskRunner*);

    float		calcDist(const Coord3&) const override;
    float		maxDist() const override;

    void		fillPar(IOPar&) const override;
    bool		usePar( const IOPar&) override;

    NotifierAccess*	getMovementNotifier() override;

    Notifier<MPEDisplay>	boxDraggerStatusChange;
    Notifier<MPEDisplay>	planeOrientationChange;

    // methods for volume-based display
    VisID		addSlice(int dim, bool show);
    visBase::OrthogonalSlice*	getSlice(int index);
    void		updateSlice();
    float		slicePosition(visBase::OrthogonalSlice*) const;
    float		getValue(const Coord3&) const;

    void		removeChild(VisID displayid);
    void		getChildren(TypeSet<VisID>&) const override;

    bool		setZAxisTransform(ZAxisTransform*,TaskRunner*) override;
    const ZAxisTransform*	getZAxisTransform() const override;

    void		setRightHandSystem(bool yn) override;

    bool		setDataVolume(int attrib,const RegularSeisDataPack*,
				      TaskRunner*) override;
    void		setTrcKeyZSampling(const TrcKeyZSampling&);

    const RegularSeisDataPack*	getCacheVolume(int attrib) const override;
    bool		setDataPackID(int attrib,DataPackID,
				      TaskRunner*) override;
    DataPackID	getDataPackID(int attrib) const override;
    virtual DataPackMgr::MgrID	getDataPackMgrID() const override
			{ return DataPackMgr::SeisID(); }

    bool		allowsPicks() const override;
    void		allowShading(bool yn) override;
    bool		hasPosModeManipulator() const override	{ return true; }
    void		showManipulator(bool yn) override;
    bool		isManipulated() const override;
    bool		canResetManipulation() const override;
    void		resetManipulation() override;
    void		acceptManipulation() override;
    BufferString	getManipulationString() const override;
    NotifierAccess*	getManipulationNotifier() override;

    static int		cInLine()	{ return 0; }
    static int		cCrossLine()	{ return 1; }
    static int		cTimeSlice()	{ return 2; }

    // texture channel-related methods

    SurveyObject::AttribFormat getAttributeFormat(int attrib=-1) const override;

    bool		canAddAttrib(int nrattribstoadd=1) const override;
    bool		canRemoveAttrib() const override;
    int			nrAttribs() const override;
    bool		addAttrib() override;
    bool		removeAttrib(int attrib) override;
    void		enableAttrib(int attrib,bool yn) override;
    bool		isAttribEnabled(int attrib) const override;

    const char*		errMsg() const override { return errmsg_.str(); }

    void		setDisplayTransformation(const mVisTrans*) override;


protected:
			~MPEDisplay();
    TrcKeyZSampling	getBoxPosition() const;
    void		setPlanePosition(const TrcKeyZSampling&);

    void		setSliceDimension(int slice,int dim);
    void		alignSliceToSurvey(visBase::OrthogonalSlice&);

    void		setSceneEventCatcher(visBase::EventCatcher*) override;

    // callback from boxdragger
    void		boxDraggerFinishCB(CallBacker*);

    // callbacks from MPE
    void		updateBoxPosition(CallBacker*);

    // methods for volume-based display
    TrcKeyZSampling	getTrcKeyZSampling(bool manippos,bool display,
					int attrib) const;

    void		triggerSel() override;
    void		triggerDeSel() override;

    bool		pickable() const	{ return true; }
    bool		rightClickable() const override	{ return false; }
    bool		selectable() const override { return false; }  // check!
    bool		isSelected() const override;

    void		turnOnSlice(bool);
    void		updateRanges(bool updateic,bool updatez);

    // callback from user
    void		mouseClickCB(CallBacker*);
    void		updateMouseCursorCB(CallBacker*) override;

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
    DataPackID		cacheid_;
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

} // namespace visSurvey
