#ifndef viswelldisplay_h
#define viswelldisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viswelldisplay.h,v 1.41 2007-10-04 12:02:05 cvsnanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"
#include "multiid.h"
#include "ranges.h"
#include "welllog.h"

class LineStyle;

namespace visBase
{
    class DataGroup;
    class EventCatcher;
    class EventInfo;
    class Transformation;
    class Well;
};

namespace Well { class Data; class Track; class LogDisplayPars; class LogDisplayParSet; }

namespace visSurvey
{
class Scene;

/*!\brief Used for displaying welltracks, markers and logs


*/

class WellDisplay :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    static WellDisplay*		create()
				mCreateDataObj(WellDisplay);

    bool			setMultiID(const MultiID&);
    MultiID			getMultiID() const 	{ return wellid_; }

    const LineStyle*		lineStyle() const;
    void			setLineStyle(const LineStyle&);

    bool			hasColor() const	{ return true; }
    Color			getColor() const;

    void			showWellTopName(bool);
    void			showWellBotName(bool);
    bool			wellTopNameShown() const;
    bool			wellBotNameShown() const;

    bool			canShowMarkers() const;
    void			showMarkers(bool);
    bool			markersShown() const;
    void			showMarkerName(bool);
    bool			markerNameShown() const;
    void			setMarkerScreenSize(int);
    int				markerScreenSize() const;

    void			displayLog(int,Interval<float>*,bool,int);
    void			displayLog(const BufferString,bool,
	    				   const Interval<float>&,int nr);
    void			displayRightLog();
    void			displayLeftLog();
    Well::LogDisplayParSet*	getLogParSet() const	{ return logparset_; }

    void			setLogColor(const Color&,int);
    const Color&		logColor(int) const;
    void			setLogLineWidth(float,int);
    float			logLineWidth(int) const;
    void			setLogWidth(int);
    int				logWidth() const;
    void			showLogs(bool);
    bool			logsShown() const;
    void			showLogName(bool);
    bool			logNameShown() const;
    const char*			logName(bool left) const
				{return left ? logparset_->getLeft()->getLogNm()					: logparset_->getRight()->getLogNm();}

    void			getMousePosInfo(const visBase::EventInfo& pos,
	    					const Coord3&,BufferString& val,
						BufferString& info) const;

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    void 			setDisplayTransformForPicks(mVisTrans*);

    void                        setSceneEventCatcher(visBase::EventCatcher*);
    void 			addPick(Coord3);
    void			setupPicking(bool);
    void			showKnownPositions();
    NotifierAccess*             getManipulationNotifier() { return &changed_; }
    bool			isHomeMadeWell() const { return picksallowed_; }
    bool			hasChanged() const 	{ return needsave_; }
    void			setChanged( bool yn )	{ needsave_ = yn; }
    TypeSet<Coord3>             getWellCoords()	const;
    				//only used for user-made wells

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

protected:

    virtual			~WellDisplay();
    void			setWell(visBase::Well*);
    void			updateMarkers(CallBacker*);
    void			fullRedraw(CallBacker*);
    TypeSet<Coord3>		getTrackPos(Well::Data*);
    void			displayLog(Well::LogDisplayPars*,int);

    void                        pickCB(CallBacker* cb=0);

    visBase::Well*		well_;

    MultiID			wellid_;
    const bool			zistime_;
    const bool			zinfeet_;

    Well::LogDisplayParSet*	logparset_;

    visBase::DataObjectGroup*	group_;
    visBase::EventCatcher*	eventcatcher_;
    mVisTrans*			transformation_;
    Notifier<WellDisplay>	changed_;

    int                         mousepressid_;
    Coord3                      mousepressposition_;

    Well::Track*		pseudotrack_;

    bool			picksallowed_;
    bool			needsave_;

    static const char*		sKeyEarthModelID;
    static const char*		sKeyWellID;
    static const char*		sKeyLog1Name;
    static const char*		sKeyLog1Range;
    static const char*		sKeyLog1Scale;
    static const char*		sKeyLog1Color;
    static const char*		sKeyLog2Name;
    static const char*		sKeyLog2Range;
    static const char*		sKeyLog2Scale;
    static const char*		sKeyLog2Color;
};

};


#endif
