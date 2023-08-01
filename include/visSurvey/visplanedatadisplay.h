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
#include "vistexturerect.h"


template <class T> class Array2D;
namespace visBase{ class GridLines; }

class BinIDValueSet;

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying an inline, crossline or timeslice.

    A PlaneDataDisplay object is the front-end object for displaying an inline,
    crossline or timeslice.  Use setOrientation(Orientation) for setting the
    requested orientation of the slice.
*/

mExpClass(visSurvey) PlaneDataDisplay :
				public visSurvey::MultiTextureSurveyObject
{ mODTextTranslationClass(PlaneDataDisplay);
public:

    typedef OD::SliceType	SliceType;
				mDeclareEnumUtils(SliceType);

				PlaneDataDisplay();

				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,
				    PlaneDataDisplay, "PlaneDataDisplay",
				     ::toUiString(sFactoryKeyword()));

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
    BufferString		getManipulationString() const override;
    NotifierAccess*		getManipulationNotifier() override;
    NotifierAccess*		getMovementNotifier() override
				{ return &movefinished_; }
    NotifierAccess*		getDataChangedNotifier()
				{ return &datachanged_; }

    bool			allowMaterialEdit() const override
				{ return true; }

    int				nrResolutions() const override;
    void			setResolution(int,TaskRunner*) override;

    SurveyObject::AttribFormat	getAttributeFormat(
						int attrib=-1) const override;

    TrcKeyZSampling		getTrcKeyZSampling(
					int attrib=-1) const override;
    void			getTraceKeyPath(TrcKeyPath&,
						TypeSet<Coord>*) const override;
    TrcKeyZSampling		getTrcKeyZSampling(bool manippos,
						bool displayspace,
						int attrib=-1) const;
    TrcKeyZSampling		getDataPackSampling(int attrib=0) const;
    Interval<float>		getDataTraceRange() const override;
    void			getRandomPos(DataPointSet&,
					TaskRunner* =nullptr) const override;
    void			setRandomPosData(int attrib,
						 const DataPointSet*,
						 TaskRunner*) override;
    void			setTrcKeyZSampling(const TrcKeyZSampling&);

    bool			setDataPackID(int attrib,DataPackID,
					      TaskRunner*) override;
    DataPackID			getDataPackID(int attrib) const override;
    DataPackID			getDisplayedDataPackID(
					      int attrib) const override;
    virtual DataPackMgr::MgrID	getDataPackMgrID() const override
				{ return DataPackMgr::SeisID(); }

    visBase::GridLines*		gridlines()		{ return gridlines_; }

    const MouseCursor*		getMouseCursor() const override
				{ return &mousecursor_; }

    void			getMousePosInfo( const visBase::EventInfo& ei,
						 IOPar& iop ) const override
				{ return MultiTextureSurveyObject
						::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
					    Coord3&,BufferString& val,
					    BufferString& info) const override;
    void			getObjectInfo(BufferString&) const override;

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

    void			annotateNextUpdateStage(bool yn) override;

    static const char*		sKeyDepthKey()		{ return "DepthKey"; }
    static const char*		sKeyPlaneKey()		{ return "PlaneKey"; }

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const visBase::TextureRectangle* getTextureRectangle() const
				{ return texturerect_;  }
    float			getZScale() const;
    const mVisTrans*		getDisplayTransformation() const override
				{ return displaytrans_; }
    bool			updatePlanePos(const TrcKeyZSampling&);
    Undo&			undo();
    const Undo&			undo() const;

protected:
				~PlaneDataDisplay();

    void			setRandomPosDataNoCache(int attrib,
							const BinIDValueSet*,
							TaskRunner*);
    void			updateChannels(int attrib,TaskRunner*);
    void			createTransformedDataPack(int attrib,
							  TaskRunner* =0);
    ConstRefMan<RegularSeisDataPack> getDataPack(int attrib) const;
    RefMan<RegularSeisDataPack> getDataPack(int attrib);
    ConstRefMan<RegularSeisDataPack> getDisplayedDataPack(int attrib) const;
    RefMan<RegularSeisDataPack> getDisplayedDataPack(int attrib);
    bool			setDataPack(int attrib,RegularSeisDataPack*,
					    TaskRunner*);
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

    TrcKeyZSampling		snapPosition(const TrcKeyZSampling&) const;
    void			updateTexShiftAndGrowth();

    visBase::EventCatcher*		eventcatcher_ = nullptr;
    MouseCursor				mousecursor_;
    RefMan<visBase::DepthTabPlaneDragger> dragger_;

    visBase::GridLines*			gridlines_;
    SliceType				orientation_ = OD::InlineSlice;

    RefObjectSet<RegularSeisDataPack>	datapacks_;
    RefObjectSet<RegularSeisDataPack>	transfdatapacks_;

    ObjectSet< TypeSet<DataPackID> >	displaycache_;
    ObjectSet<BinIDValueSet>		rposcache_;

    TrcKeyZSampling			csfromsession_;
    BinID				curicstep_;
    Notifier<PlaneDataDisplay>		moving_;
    Notifier<PlaneDataDisplay>		movefinished_;
    Notifier<PlaneDataDisplay>		datachanged_;

    ZAxisTransform*			datatransform_ = nullptr;
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
	SliceType	oldorientation_ = OD::InlineSlice;
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
