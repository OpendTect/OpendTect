#ifndef viswelldisplay_h
#define viswelldisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/


#include "vissurveymod.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "viswell.h"
#include "multiid.h"
#include "ranges.h"
#include "welllogdisp.h"

class BaseMapObject;
class LineStyle;

namespace visBase
{
    class DataObjectGroup;
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

mClass(visSurvey) WellDisplay : public visBase::VisualObjectImpl
		   , public visSurvey::SurveyObject
{
public:
    static WellDisplay*		create()
				mCreateDataObj(WellDisplay);

    bool			setMultiID(const MultiID&);
    MultiID			getMultiID() const 	{ return wellid_; }

    //track
    void 			fillTrackParams(visBase::Well::TrackParams&);

    bool			wellTopNameShown() const;
    void			showWellTopName(bool);
    bool			wellBotNameShown() const;
    void			showWellBotName(bool);
    TypeSet<Coord3>             getWellCoords()	const;

    //markers
    void 			fillMarkerParams(visBase::Well::MarkerParams&);

    bool			canShowMarkers() const;
    bool			markersShown() const;
    void			showMarkers(bool);
    bool			markerNameShown() const;
    void			showMarkerName(bool);
    int				markerScreenSize() const;
    void			setMarkerScreenSize(int);
    
    //logs
    void 			fillLogParams(visBase::Well::LogParams&,int);

    const LineStyle*		lineStyle() const;
    void			setLineStyle(const LineStyle&);
    bool			hasColor() const	{ return true; }
    Color			getColor() const;
    void 			setLogData(visBase::Well::LogParams&,bool);
    void			setLogDisplay(int);
    void			calcClippedRange(float,Interval<float>&,int);
    void			displayRightLog();
    void			displayLeftLog();
    void			setOneLogDisplayed(bool);
    const Color&		logColor(int) const;
    void			setLogColor(const Color&,int);
    float			logLineWidth(int) const;
    void			setLogLineWidth(float,int);
    int				logWidth() const;
    void			setLogWidth(int,int);
    bool			logsShown() const;
    void			showLogs(bool);
    bool			logNameShown() const;
    void			showLogName(bool);
    void                        setLogConstantSize(bool);
    bool                        logConstantSize() const;


    const mVisTrans*		getDisplayTransformation() const;
    void			setDisplayTransformation(const mVisTrans*);
    void 			setDisplayTransformForPicks(const mVisTrans*);
    
    void                        setSceneEventCatcher(visBase::EventCatcher*);
    void 			addPick(Coord3);
    				//only used for user-made wells
    void			addKnownPos();
    void			getMousePosInfo(const visBase::EventInfo& ei,
	    					IOPar& iop ) const
				{ return SurveyObject::getMousePosInfo(ei,iop);}
    void			getMousePosInfo(const visBase::EventInfo& pos,
	    					Coord3&,BufferString& val,
						BufferString& info) const;
    NotifierAccess*             getManipulationNotifier() { return &changed_; }
    bool			hasChanged() const 	{ return needsave_; }
    bool			isHomeMadeWell() const { return picksallowed_; }
    void			setChanged( bool yn )	{ needsave_ = yn; }
    void			setupPicking(bool);
    void			showKnownPositions();
    void                        restoreDispProp();
    Well::Data*			getWD() const;

    bool			allowsPicks() const	{ return true; }

    bool			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    const ZAxisTransform*	getZAxisTransform() const;
    
    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);
    const char*			errMsg() const { return errmsg_.str(); }

protected:

    virtual			~WellDisplay();

    BaseMapObject*		createBaseMapObject();
    void			setWell(visBase::Well*);
    void			updateMarkers(CallBacker*);
    void			fullRedraw(CallBacker*);
    void			getTrackPos(const Well::Data*,TypeSet<Coord3>&);
    void			displayLog(Well::LogDisplayPars*,int);
    void			setLogProperties(visBase::Well::LogParams&);
    void                        pickCB(CallBacker* cb=0);
    void                        welldataDelNotify(CallBacker* cb=0);
    void 			saveDispProp( const Well::Data* wd );
    void			setLogInfo(BufferString&,BufferString&,
					   float,bool) const;
    
    Well::DisplayProperties* 	dispprop_;

    Coord3                      mousepressposition_;
    const mVisTrans*		transformation_;
    MultiID			wellid_;
    visBase::EventCatcher*	eventcatcher_;
    visBase::DataObjectGroup*	group_;
    visBase::Well*		well_;
    Well::Track*		pseudotrack_;
    Well::Data*			wd_;

    ZAxisTransform*		datatransform_;
    void			dataTransformCB(CallBacker*);

    Notifier<WellDisplay>	changed_;

    int 			logsnumber_;
    int                         mousepressid_;
    bool			needsave_;
    bool			onelogdisplayed_;
    bool			picksallowed_;
    const bool			zistime_;
    const bool			zinfeet_;
    static const char*		sKeyEarthModelID;
    static const char*		sKeyWellID;
};

} // namespace visSurvey


#endif

