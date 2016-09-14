#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/


#include "vissurveymod.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "viswell.h"
#include "dbkey.h"
#include "ranges.h"
#include "welllogdisp.h"
#include "factory.h"

namespace OD { class LineStyle; }

namespace visBase
{
    class MarkerSet;
    class EventCatcher;
    class EventInfo;
    class Transformation;
}

namespace Well
{
    class Data;
    class DisplayProperties;
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

    bool			setDBKey(const DBKey&);
    DBKey			getDBKey() const	{ return wellid_; }

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

    const OD::LineStyle*		lineStyle() const;
    void			setLineStyle(const OD::LineStyle&);
    bool			hasColor() const	{ return true; }
    Color			getColor() const;
    void			setLogData(visBase::Well::LogParams&,bool);
    void			setLogDisplay(visBase::Well::Side);
    void			calcClippedRange(float,Interval<float>&,int);
    void			displayRightLog();
    void			displayLeftLog();
    void			setOneLogDisplayed(bool);
    const Color&		logColor(visBase::Well::Side) const;
    void			setLogColor(const Color&,visBase::Well::Side);
    float			getLogWidth(visBase::Well::Side) const;
    void			setLogWidth(float,visBase::Well::Side);
    int				getLogLineWidth() const;
    void			setLogLineWidth(int,visBase::Well::Side);
    bool			logsShown() const;
    void			showLogs(bool);
    bool			logNameShown() const;
    void			showLogName(bool);

    const mVisTrans*		getDisplayTransformation() const;
    void			setDisplayTransformation(const mVisTrans*);
    void			setDisplayTransformForPicks(const mVisTrans*);

    void			setSceneEventCatcher(visBase::EventCatcher*);
    void			addPick(const Coord3&);
				//only used for user-made wells
    void			addKnownPos();
    void			getMousePosInfo(const visBase::EventInfo& ei,
						IOPar& iop ) const
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo& pos,
						Coord3&,BufferString& val,
						BufferString& info) const;
    NotifierAccess*		getManipulationNotifier() { return &changed_; }
    bool			hasChanged() const	{ return needsave_; }
    bool			isHomeMadeWell() const { return picksallowed_; }
    void			setChanged( bool yn )	{ needsave_ = yn; }
    void			setupPicking(bool);
    void			showKnownPositions();
    void			restoreDispProp();
    Well::Data*			getWD() const;
    bool			needsConversionToTime() const;

    bool			allowsPicks() const	{ return true; }

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);
    virtual void		setPixelDensity(float);
    const char*			errMsg() const { return errmsg_.str(); }
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
    void			welldataDelNotify(CallBacker* cb=0);
    void			saveDispProp( const Well::Data* wd );
    void			setLogInfo(BufferString&,BufferString&,
					   float,bool) const;
    bool			upscaleLogs(const Well::Track&,Well::Log&,
					    Well::Log*) const;
    void			removePick(const visBase::EventInfo&);
    void			addPick(const visBase::EventInfo&,int);

    Well::DisplayProperties*	dispprop_;

    Coord3			mousepressposition_;
    const mVisTrans*		transformation_;
    DBKey			wellid_;
    visBase::EventCatcher*	eventcatcher_;
    visBase::MarkerSet*		markerset_;
    visBase::Well*		well_;
    Well::Track*		pseudotrack_;
    Well::Track*		timetrack_;
    Well::Data*			wd_;

    ZAxisTransform*		datatransform_;
    void			dataTransformCB(CallBacker*);

    Notifier<WellDisplay>	changed_;

    int				logsnumber_;
    int				mousepressid_;
    bool			needsave_;
    bool			onelogdisplayed_;
    bool			picksallowed_;
    const bool			zistime_;
    const bool			zinfeet_;
    static const char*		sKeyEarthModelID;
    static const char*		sKeyWellID;
};

} // namespace visSurvey


