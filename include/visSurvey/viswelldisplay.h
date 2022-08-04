#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/


#include "vissurveymod.h"

#include "factory.h"
#include "multiid.h"
#include "ranges.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "viswell.h"
#include "welldata.h"
#include "welllogdisp.h"


namespace visBase
{
    class MarkerSet;
    class EventCatcher;
    class EventInfo;
    class Transformation;
}

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

/*!\brief Used for displaying welltracks, markers and logs


*/

mExpClass(visSurvey) WellDisplay : public visBase::VisualObjectImpl
		   , public visSurvey::SurveyObject
{
public:
				WellDisplay();
				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,WellDisplay,
				    "WellDisplay",
				    toUiString(sFactoryKeyword()) )

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
					    BufferString& info) const override;
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
    const visBase::Well*	getWell() const { return well_; }

protected:
				~WellDisplay();

    void			setWell(visBase::Well*);
    void			updateMarkers(CallBacker*);
    void			fullRedraw(CallBacker*);
    void			getTrackPos(const Well::Data*,TypeSet<Coord3>&);
    void			displayLog(Well::LogDisplayPars*,int);
    void			setLogProperties(visBase::Well::LogParams&);
    void			pickCB(CallBacker* cb=0);
    void			saveDispProp( const Well::Data* wd );
    void			setLogInfo(BufferString&,BufferString&,
					   float,bool) const;
    void			removePick(const visBase::EventInfo&);
    void			addPick(const visBase::EventInfo&,int);

    Well::DisplayProperties*	dispprop_		= nullptr;

    Coord3			mousepressposition_;
    const mVisTrans*		transformation_		= nullptr;
    MultiID			wellid_			= MultiID::udf();
    visBase::EventCatcher*	eventcatcher_		= nullptr;
    visBase::MarkerSet*		markerset_		= nullptr;
    visBase::Well*		well_			= nullptr;
    Well::Track*		pseudotrack_		= nullptr;
    Well::Track*		timetrack_		= nullptr;
    RefMan<Well::Data>		wd_;

    ZAxisTransform*		datatransform_		= nullptr;
    void			dataTransformCB(CallBacker*);

    Notifier<WellDisplay>	changed_;

    int				logsnumber_		= 0;
    int				mousepressid_		= -1;
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


