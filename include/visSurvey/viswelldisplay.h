#ifndef viswelldisplay_h
#define viswelldisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viswelldisplay.h,v 1.34 2006-01-16 15:45:31 cvshelene Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"
#include "multiid.h"
#include "ranges.h"

class LineStyle;

namespace visBase
{
    class DataGroup;
    class EventCatcher;
    class EventInfo;
    class Transformation;
    class Well;
};

namespace Well { class Data; class Track; }

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

    bool			setWellId(const MultiID&);
    const MultiID&		wellId() const 		{ return wellid_; }

    const LineStyle*		lineStyle() const;
    void			setLineStyle(const LineStyle&);

    bool			hasColor() const	{ return true; }
    Color			getColor() const;

    void			showWellName(bool);
    bool			wellNameShown() const;

    bool			canShowMarkers() const;
    void			showMarkers(bool);
    bool			markersShown() const;
    void			showMarkerName(bool);
    bool			markerNameShown() const;
    void			setMarkerScreenSize(int);
    int				markerScreenSize() const;

    void			displayLog(int idx,int nr,bool logarthm_scale,
	    				   const Interval<float>* rg=0);
    				//!< idx: idx in Well::LogSet
    				//!< nr==1: left log; nr==2: right log
    void			displayLog(const char*,bool,
	    				   const Interval<float>&,int nr);
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

    void			getMousePosInfo(const visBase::EventInfo& pos,
	    					const Coord3&,float& val,
						BufferString& info) const;

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    void 			setDisplayTransformForPicks(mVisTrans*);

    void                        setSceneEventCatcher(visBase::EventCatcher*);
    void 			addPick(Coord3);
    void			setupPicking();
    void			lock(bool yn)		{ locked_ = yn; }
    void			showKnownPositions();
    NotifierAccess*             getManipulationNotifier() { return &changed_; }
    bool			isHomeMadeWell() const { return picksallowed_; }
    TypeSet<Coord3>             getWellCoords()	const;
    				//only used for user-made wells

protected:

    virtual			~WellDisplay();
    void			setWell(visBase::Well*);
    void			updateMarkers(CallBacker*);
    void			fullRedraw(CallBacker*);
    TypeSet<Coord3>		getTrackPos(Well::Data*);

    void                        pickCB(CallBacker* cb=0);

    visBase::Well*		well_;

    MultiID			wellid_;
    const bool			zistime_;
    const bool			zinfeet_;

    BufferString		log1nm_;
    BufferString		log2nm_;
    Interval<float>		log1rg_;
    Interval<float>		log2rg_;
    bool			log1logsc_;
    bool			log2logsc_;

    visBase::DataObjectGroup*	group_;
    visBase::EventCatcher*	eventcatcher_;
    mVisTrans*			transformation_;
    Notifier<WellDisplay>	changed_;

    int                         mousepressid_;
    Coord3                      mousepressposition_;

    Well::Track*		pseudotrack_;

    bool			picksallowed_;
    bool			locked_;

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
