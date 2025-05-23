#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vismultiattribsurvobj.h"

#include "mousecursor.h"
#include "odcommonenums.h"
#include "seisdatapack.h"
#include "undo.h"
#include "visdepthtabplanedragger.h"
#include "visevent.h"
#include "visgridlines.h"
#include "vistexturerect.h"
#include "zaxistransform.h"


template <class T> class Array2D;

class BinIDValueSet;

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying an inline, crossline or timeslice.

    A PlaneDataDisplay object is the front-end object for displaying an inline,
    crossline or timeslice.  Use setOrientation(Orientation) for setting the
    requested orientation of the slice.
*/

mExpClass(visSurvey) PlaneDataDisplay : public MultiTextureSurveyObject
{ mODTextTranslationClass(PlaneDataDisplay);
public:

    typedef OD::SliceType	SliceType;
				mDeclareEnumUtils(SliceType);

				PlaneDataDisplay();

				mDefaultFactoryInstantiation(
				    SurveyObject, PlaneDataDisplay,
				    "PlaneDataDisplay",
				     ::toUiString(sFactoryKeyword()) )

    bool			isInlCrl() const override	{ return true; }

    void			setOrientation(SliceType);
    SliceType			getOrientation() const { return orientation_; }

    bool			hasPosModeManipulator() const override
				{ return true; }
    void			showManipulator(bool) override;
    bool			isManipulatorShown() const override;
    bool			isManipulated() const override;
    bool			canResetManipulation() const override
				{ return true; }
    void			resetManipulation() override;
    void			acceptManipulation() override;
    uiString			getManipulationString() const override;
    NotifierAccess*		getManipulationNotifier() override;
    NotifierAccess*		getMovementNotifier() override
				{ return &movefinished_; }
    NotifierAccess*		getDataChangedNotifier()
				{ return &datachanged_; }

    bool			allowMaterialEdit() const override
				{ return true; }

    int				nrResolutions() const override;
    void			setResolution(int,TaskRunner*) override;

    SurveyObject::AttribFormat	getAttributeFormat(int attrib=-1)
							    const override;

    TrcKeyZSampling		getTrcKeyZSampling(bool displayspace=false,
					int attrib=-1) const override;
    void			getTraceKeyPath(TrcKeySet&,
					    TypeSet<Coord>*) const override;
    TrcKeyZSampling		getTrcKeyZSampling(bool manippos,
						bool displayspace,
						int attrib=-1) const;
    TrcKeyZSampling		getDataPackSampling(int attrib=0) const;
    Interval<float>		getDataTraceRange() const override;
    bool			getRandomPos(DataPointSet&,
					TaskRunner* =nullptr) const override;
    bool			setRandomPosData(int attrib,
						 const DataPointSet*,
						 TaskRunner*) override;
    void			setTrcKeyZSampling(
					    const TrcKeyZSampling&) override;

    ConstRefMan<VolumeDataPack> getDisplayedVolumeDataPack(int attrib)
								const override;
    RefMan<VolumeDataPack>	getDisplayedVolumeDataPack(int attrib);

    visBase::GridLines*		gridlines();

    const MouseCursor*		getMouseCursor() const override
				{ return &mousecursor_; }

    void			getMousePosInfo( const visBase::EventInfo& ei,
						 IOPar& iop ) const override
				{ return MultiTextureSurveyObject
						::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
					    Coord3&,BufferString& val,
					    uiString& info) const override;
    void			getObjectInfo(uiString&) const override;

    float			calcDist(const Coord3&) const override;
    float			maxDist() const override;
    Coord3			getNormal(const Coord3&) const override;
    bool			allowsPicks() const override	{ return true; }

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;
    const ZAxisTransform*	getZAxisTransform() const override;

    void			setTranslationDragKeys(bool depth,int keys);
				/*!<\param depth specifies wheter the depth or
						 the plane setting should be
						 changed.
				    \param keys   combination of OD::ButtonState
				    \note only shift/ctrl/alt are used. */

    int				getTranslationDragKeys(bool depth) const;
				/*!<\param depth specifies wheter the depth or
						 the plane setting should be
						 returned.
				    \returns	combination of OD::ButtonState*/
    bool			isVerticalPlane() const override;

    bool			canDuplicate() const override	{ return true; }
    SurveyObject*		duplicate(TaskRunner*) const override;
    PlaneDataDisplay*		createTransverseSection(const Coord3& pos,
						OD::SliceType slicetype) const;

    void			annotateNextUpdateStage(bool yn) override;

    static const char*		sKeyDepthKey()		{ return "DepthKey"; }
    static const char*		sKeyPlaneKey()		{ return "PlaneKey"; }

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const visBase::TextureRectangle* getTextureRectangle() const;
    float			getZScale() const;
    const mVisTrans*		getDisplayTransformation() const override;
    bool			updatePlanePos(const TrcKeyZSampling&);
    Undo&			undo();
    const Undo&			undo() const;

    void			setEditMode(bool yn)	{ ineditmode_ = yn; };
    bool			inEditMode() const	{ return ineditmode_; }

protected:
				~PlaneDataDisplay();

    bool			setRandomPosDataNoCache(int attrib,
							const BinIDValueSet*,
							TaskRunner*);
    void			updateChannels(int attrib,TaskRunner*);
    void			createTransformedDataPack(int attrib,
							  TaskRunner* =nullptr);

    bool			usesDataPacks() const override	{ return true; }
    bool			setVolumeDataPack(int attrib,VolumeDataPack*,
						TaskRunner*) override;
    ConstRefMan<DataPack>	getDataPack(int attrib) const override;
    ConstRefMan<VolumeDataPack> getVolumeDataPack(int attrib) const override;
    RefMan<VolumeDataPack>	getVolumeDataPack(int attrib);

    void			updateMainSwitch();
    void			setScene(Scene*) override;
    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;
    void			updateRanges(bool resetpos=false);
    void			updateRanges(bool resetinlcrl=false,
					     bool resetz=false);
    void			manipChanged(CallBacker*);
    void			coltabChanged(CallBacker*);
    void			draggerStart(CallBacker*);
    void			draggerMotion(CallBacker*);
    void			draggerFinish(CallBacker*);
    void			draggerRightClick(CallBacker*);
    void			setDraggerPos(const TrcKeyZSampling&);
    void			dataTransformCB(CallBacker*);
    void			updateMouseCursorCB(CallBacker*) override;

    bool			getCacheValue(int attrib,int version,
					      const Coord3&,
					      float&) const override;
    void			addCache() override;
    void			removeCache(int) override;
    void			swapCache(int,int) override;
    void			emptyCache(int) override;
    bool			hasCache(int) const override;

    TrcKeyZSampling		snapPosition(const TrcKeyZSampling&,
					     bool onlyic=false) const;
    void			updateTexShiftAndGrowth();

    RefMan<visBase::EventCatcher>	eventcatcher_;
    MouseCursor				mousecursor_;
    RefMan<visBase::DepthTabPlaneDragger> dragger_;
    bool				ineditmode_	= false;

    RefMan<visBase::GridLines>		gridlines_;
    SliceType				orientation_ = OD::SliceType::Inline;

    RefObjectSet<RegularSeisDataPack>	datapacks_;
    RefObjectSet<RegularSeisDataPack>	transformedpacks_;
    ObjectSet<BinIDValueSet>		rposcache_;

    TrcKeyZSampling			csfromsession_;
    BinID				curicstep_;
    Notifier<PlaneDataDisplay>		moving_;
    Notifier<PlaneDataDisplay>		movefinished_;
    Notifier<PlaneDataDisplay>		datachanged_;

    RefMan<ZAxisTransform>		datatransform_;
    int					voiidx_ = -1;

    ConstRefMan<mVisTrans>		displaytrans_;
    RefMan<visBase::TextureRectangle>	texturerect_;

    int					originalresolution_ = -1;
    bool				forcemanipupdate_ = false;
    bool				interactivetexturedisplay_ = false;

    struct UpdateStageInfo
    {
	bool		refreeze_ = true;
	TrcKeyZSampling oldcs_;
	SliceType	oldorientation_ = OD::SliceType::Inline;
    };
    UpdateStageInfo		updatestageinfo_;
    TrcKeyZSampling		startmovepos_;
    Undo&			undo_;

    static const char*		sKeyOrientation() { return "Orientation"; }
    static const char*		sKeyResolution()  { return "Resolution"; }
    static const char*		sKeyGridLinesID() { return "GridLines ID"; }

    void			updateTexOriginAndScale(int attrib,
							const TrcKeyZSampling&);
};

} // namespace visSurvey
