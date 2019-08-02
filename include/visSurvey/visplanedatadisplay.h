#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
________________________________________________________________________

-*/

#include "vissurveycommon.h"
#include "vismultiattribsurvobj.h"

#include "mousecursor.h"
#include "oduicommon.h"
#include "undo.h"
#include "visdepthtabplanedragger.h"
#include "vistexturerect.h"
#include "vistransform.h"

template <class T> class Array2D;
namespace visBase
{
    class GridLines;
}

class BinnedValueSet;
class Probe;

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
				    SurveyObject,
				    PlaneDataDisplay, "PlaneDataDisplay",
				     ::toUiString(sFactoryKeyword()));
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }

    bool			isInlCrl() const { return true; }

    void			setOrientation(SliceType);
    SliceType			getOrientation() const { return orientation_; }

    bool			hasPosModeManipulator() const	{ return true; }
    void			showManipulator(bool);
    bool			isManipulatorShown() const;
    bool			isManipulated() const;
    bool			canResetManipulation() const	{ return true; }
    void			resetManipulation();
    void			acceptManipulation();
    BufferString		getManipulationString() const;
    NotifierAccess*		getManipulationNotifier();
    NotifierAccess*		getMovementNotifier()
				{ return &movefinished_; }
    NotifierAccess*		getDataChangedNotifier()
				{ return &datachanged_; }
    NotifierAccess*		posChanged()
				{ return &poschanged_; }

    bool			allowMaterialEdit() const	{ return true; }
    bool			isSection() const { return true; }

    int				nrResolutions() const;
    void			setResolution(int,TaskRunner*);

    SurveyObject::AttribFormat	getAttributeFormat(int attrib=-1) const;

    TrcKeyZSampling		getTrcKeyZSampling(int attrib=-1) const;
    void			getTraceKeyPath(TrcKeyPath&,
                                                TypeSet<Coord>*) const;
    TrcKeyZSampling		getTrcKeyZSampling(bool manippos,
						bool displayspace,
						int attrib=-1) const;
    TrcKeyZSampling		getDataPackSampling(int attrib=0) const;
    Interval<float>		getDataTraceRange() const;
    void			getRandomPos(DataPointSet&,TaskRunner* =0)const;
    void			setRandomPosData(int,const DataPointSet*,
						 const TaskRunnerProvider&);
    void			setTrcKeyZSampling(const TrcKeyZSampling&);

    bool			setDataPackID(int attrib,DataPack::ID,
					      TaskRunner*);
    DataPack::ID		getDataPackID(int attrib) const;
    DataPack::ID		getDisplayedDataPackID(int attrib) const;
    virtual DataPackMgr::ID	getDataPackMgrID() const
				{ return DataPackMgr::SeisID(); }

    visBase::GridLines*		gridlines()		{ return gridlines_; }

    const MouseCursor*		getMouseCursor() const { return &mousecursor_; }

    void			getMousePosInfo(const visBase::EventInfo& ei,
						IOPar& iop ) const
				{ return MultiTextureSurveyObject
						::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo&,
						Coord3&,
						BufferString& val,
						BufferString& info) const;
    void			getObjectInfo(BufferString&) const;

    virtual float		calcDist(const Coord3&) const;
    virtual float		maxDist() const;
    virtual Coord3		getNormal(const Coord3&) const;
    virtual bool		allowsPicks() const		{ return true; }

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    void			setTranslationDragKeys(bool depth, int );
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
    bool			isVerticalPlane() const;

    virtual bool		canDuplicate() const	{ return true; }
    virtual SurveyObject*	duplicate(TaskRunner*) const;

    virtual void		annotateNextUpdateStage(bool yn);

    static const char*		sKeyDepthKey()		{ return "DepthKey"; }
    static const char*		sKeyPlaneKey()		{ return "PlaneKey"; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    void			setDisplayTransformation(const mVisTrans*);
    const visBase::TextureRectangle* getTextureRectangle() const
				{ return texturerect_;  }
    float			getZScale() const;
    const mVisTrans*		getDisplayTransformation() const
				{ return displaytrans_; }
    bool			updatePlanePos(const TrcKeyZSampling&);
    Undo&			undo();
    const Undo&			undo() const;

    void			setProbe(Probe*);
    Probe*			getProbe()		{ return probe_; }
    const Probe*		getProbe() const	{ return probe_; }

protected:
				~PlaneDataDisplay();

    void			setRandomPosDataNoCache(int,
						const BinnedValueSet*,
						const TaskRunnerProvider&);
    void			updateChannels(int,const TaskRunnerProvider&);
    void			createTransformedDataPack(int attrib,
							  TaskRunner* =0);
    void			updateMainSwitch();
    void			setScene(Scene*);
    void			setSceneEventCatcher(visBase::EventCatcher*);
    void			setRightHandSystem(bool);
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
    void			updateMouseCursorCB(CallBacker*);

    bool			getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;
    void			addCache();
    void			removeCache(int);
    void			swapCache(int,int);
    void			emptyCache(int);
    bool			hasCache(int) const;

    TrcKeyZSampling		snapPosition(const TrcKeyZSampling&) const;

    void			updateTexShiftAndGrowth();
    void			updateTexOriginAndScale(int attrib,
							const TrcKeyZSampling&);

    visBase::EventCatcher*		eventcatcher_;
    MouseCursor&			mousecursor_;
    RefMan<visBase::DepthTabPlaneDragger> dragger_;

    visBase::GridLines*			gridlines_;
    SliceType				orientation_;

    RefMan<Probe>			probe_;
    TypeSet<DataPack::ID>		datapackids_;
    TypeSet<DataPack::ID>		transfdatapackids_;

    ObjectSet< TypeSet<DataPack::ID> >	displaycache_;
    ObjectSet<BinnedValueSet>		rposcache_;

    TrcKeyZSampling			csfromsession_;
    BinID				curicstep_;
    Notifier<PlaneDataDisplay>		moving_;
    Notifier<PlaneDataDisplay>		movefinished_;
    Notifier<PlaneDataDisplay>		datachanged_;
    Notifier<PlaneDataDisplay>		poschanged_;

    ZAxisTransform*			datatransform_;
    int					voiidx_;

    ConstRefMan<mVisTrans>		displaytrans_;
    RefMan<visBase::TextureRectangle>	texturerect_;

    int					originalresolution_;
    bool				forcemanipupdate_;
    bool				interactivetexturedisplay_;

    struct UpdateStageInfo
    {
	bool		refreeze_;
	TrcKeyZSampling oldtkzs_;
	SliceType	oldorientation_;
    };
    UpdateStageInfo			updatestageinfo_;
    TrcKeyZSampling			startmovepos_;
    Undo&				undo_;

    static const char*		sKeyOrientation() { return "Orientation"; }
    static const char*		sKeyResolution()  { return "Resolution"; }
    static const char*		sKeyGridLinesID() { return "GridLines ID"; }
};

} // namespace visSurvey
