#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "factory.h"
#include "multiid.h"
#include "ranges.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"
#include "viswell.h"
#include "welldata.h"
#include "welllogdisp.h"
#include "zaxistransform.h"


namespace Well
{
    class DisplayProperties;
    class LoadReqs;
    class Log;
    class LogDisplayPars;
    class Track;
}

namespace visSurvey
{

class Scene;

/*!\brief Used for displaying welltracks, markers and logs */

mExpClass(visSurvey) WellDisplay : public visBase::VisualObjectImpl
				 , public SurveyObject
{ mODTextTranslationClass(WellDisplay)
public:
				WellDisplay();

				mDefaultFactoryInstantiation(
				    SurveyObject, WellDisplay,
				    "WellDisplay",
				    ::toUiString(sFactoryKeyword()) )

    bool			setMultiID(const MultiID&);
    MultiID			getMultiID() const override { return wellid_; }

    //track
    void			fillTrackParams(visBase::Well::TrackParams&);

    bool			wellTopNameShown() const;
    void			showWellTopName(bool);
    bool			wellBotNameShown() const;
    void			showWellBotName(bool);
    TypeSet<Coord3>		getWellCoords() const;

    //markers
    void			fillMarkerParams(visBase::Well::MarkerParams&);

    bool			canShowMarkers() const;
    bool			markersShown() const;
    void			showMarkers(bool);
    bool			markerNameShown() const;
    void			showMarkerName(bool);
    int				markerScreenSize() const;
    void			setMarkerScreenSize(int);

    //logs
    void			fillLogParams(visBase::Well::LogParams&,
						visBase::Well::Side side);

    const OD::LineStyle*	lineStyle() const override;
    void			setLineStyle(const OD::LineStyle&) override;
    bool			hasColor() const override	{ return true; }
    OD::Color			getColor() const override;
    void			setLogData(visBase::Well::LogParams&,bool);
    void			setLogDisplay(visBase::Well::Side);
    void			calcClippedRange(float,Interval<float>&,int);
    void			displayRightLog();
    void			displayLeftLog();
    void			displayCenterLog();
    void			setOneLogDisplayed(bool);
    const OD::Color&		logColor(visBase::Well::Side) const;
    void			setLogColor(const OD::Color&,
							visBase::Well::Side);
    float			getLogWidth(visBase::Well::Side) const;
    void			setLogWidth(float,visBase::Well::Side);
    int				getLogLineWidth() const;
    void			setLogLineWidth(int,visBase::Well::Side);
    bool			logsShown() const;
    void			showLogs(bool);
    bool			logNameShown() const;
    void			showLogName(bool);

    void			setResolution(int res,TaskRunner*) override;
    int				getResolution() const override;
    int				nrResolutions() const override;
    BufferString		getResolutionName(int res) const override;

    const mVisTrans*		getDisplayTransformation() const override;
    void			setDisplayTransformation(
					const mVisTrans*) override;
    void			setDisplayTransformForPicks(const mVisTrans*);

    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;
    void			addPick(Coord3);
				//only used for user-made wells
    void			addKnownPos();
    void			getMousePosInfo( const visBase::EventInfo& ei,
						 IOPar& iop ) const override
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo& pos,
					    Coord3&,BufferString& val,
					    uiString& info) const override;
    NotifierAccess*		getManipulationNotifier() override
				{ return &changed_; }
    bool			hasChanged() const	{ return needsave_; }
    bool			isHomeMadeWell() const { return picksallowed_; }
    void			setChanged( bool yn )	{ needsave_ = yn; }
    void			setupPicking(bool);
    void			showKnownPositions();
    void			restoreDispProp();
    RefMan<Well::Data>		getWD(const Well::LoadReqs&) const;
    bool			needsConversionToTime() const;

    bool			allowsPicks() const override	{ return true; }

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;
    const ZAxisTransform*	getZAxisTransform() const override;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;
    void			setPixelDensity(float) override;
    const char*			errMsg() const  override
				{ return errmsg_.str(); }
    const visBase::Well*	getWell() const { return well_.ptr(); }

protected:
				~WellDisplay();

    void			setWell(visBase::Well*);
    void			updateMarkers(CallBacker*);
    void			fullRedraw(CallBacker*);
    void			getTrackPos(const Well::Data*,TypeSet<Coord3>&);
    void			displayLog(Well::LogDisplayPars*,int);
    void			setLogProperties(visBase::Well::LogParams&);
    void			pickCB(CallBacker* =nullptr);
    void			saveDispProp(const Well::Data*);
    void			setLogInfo(uiString&,BufferString&,
					   float,visBase::Well::Side) const;
    void			removePick(const visBase::EventInfo&);
    void			addPick(const visBase::EventInfo&,const VisID&);

    Well::DisplayProperties*	dispprop_		= nullptr;

    Coord3			mousepressposition_;
    ConstRefMan<mVisTrans>	transformation_;
    MultiID			wellid_;
    RefMan<visBase::EventCatcher> eventcatcher_;
    RefMan<visBase::MarkerSet>	markerset_;
    RefMan<visBase::Well>	well_;
    Well::Track*		pseudotrack_		= nullptr;
    Well::Track*		timetrack_		= nullptr;
    RefMan<Well::Data>		wd_;

    RefMan<ZAxisTransform>	datatransform_;
    void			dataTransformCB(CallBacker*);

    Notifier<WellDisplay>	changed_;

    int				logsnumber_		= 0;
    VisID			mousepressid_;
    bool			needsave_		= false;
    bool			onelogdisplayed_	= false;
    bool			picksallowed_		= false;
    int				logresolution_ = 2; // 1/4 of full resolution
    const bool			zistime_;
    const bool			zinfeet_;

    static const char*		sKeyEarthModelID;
    static const char*		sKeyWellID;
};

} // namespace visSurvey
