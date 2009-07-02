#ifndef viswelldisplay_h
#define viswelldisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viswelldisplay.h,v 1.52 2009-07-02 20:59:44 cvskris Exp $



-*/


#include "visobject.h"
#include "vissurvobj.h"
#include "multiid.h"
#include "ranges.h"
#include "welllogdisp.h"

class LineStyle;
namespace Well { class Log; class LogDisplayPars; class LogDisplayParSet; }


namespace visBase
{
    class DataObjectGroup;
    class EventCatcher;
    class EventInfo;
    class Transformation;
    class Well;
}

namespace Well
{
    class Data;
    class Log;
    class LogDisplayPars;
    class LogDisplayParSet;
    class Track;
}

namespace visSurvey
{
class Scene;

/*!\brief Used for displaying welltracks, markers and logs


*/

mClass WellDisplay :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    static WellDisplay*		create()
				mCreateDataObj(WellDisplay);

    bool			setMultiID(const MultiID&);
    MultiID			getMultiID() const 	{ return wellid_; }

    const LineStyle*		lineStyle() const;
    void			setLineStyle(LineStyle);

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
    
    void 			setWellData( const int, TypeSet<Coord3Value>&,
	                           Interval<float>*, Interval<float>&,
				   bool&, Well::Log& );
    void			createLogDisplay(int,Interval<float>*,bool,int);
    void			createFillLogDisplay(int,Interval<float>*,
	    							bool,int);
    void			setLogDisplay(Well::LogDisplayPars&,int);
    void			displayLog(const BufferString, bool,
	    				       Interval<float>&,int nr);
    void			calcClippedRange(float,Interval<float>&,
	    							Well::Log&);
    void			displayRightLog();
    void			displayLeftLog();
    Well::LogDisplayParSet*	getLogParSet() const	{ return &logparset_; }

    void			setOneLogDisplayed(bool);
    	
    void			setLogColor(const Color&,int);
    const Color&		logColor(int) const;
    void			setLogFillColor(const Color&,int, const char*
	    					,const bool, const bool);
    const Color&		logFillColor(int) const;
    void			setLogLineWidth(float,int);
    float			logLineWidth(int) const;
    void			setLogWidth(int,int);
    int				logWidth() const;
    void			showLogs(bool);
    bool			logsShown() const;
    void			showLogName(bool);
    bool			logNameShown() const;
    const char*			logName(bool left) const
				{return left ? logparset_.getLeft()->name_					: logparset_.getRight()->name_;}

    void			getMousePosInfo(const visBase::EventInfo& pos,
	    					Coord3&,BufferString& val,
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
    void			setWellProperties(int,Interval<float>&);
    void                        pickCB(CallBacker* cb=0);

    visBase::Well*		well_;

    MultiID			wellid_;
    const bool			zistime_;
    const bool			zinfeet_;

    bool			onelogdisplayed_;
    int 			logsnumber_;
    Well::LogDisplayParSet&	logparset_;

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
    static const char*		sKeyLog1Repeat;
    static const char*		sKeyLog1Ovlap;
    static const char*		sKeyLog1Clip;
    static const char*		sKeyLog1Style;
    static const char*		sKeyLog1Color;
    static const char*		sKeyLog1FillColor;
    static const char*		sKeyLog1SeisFillColor;
    static const char*		sKeyLog2Name;
    static const char*		sKeyLog2Range;
    static const char*		sKeyLog2Repeat;
    static const char*		sKeyLog2Ovlap;
    static const char*		sKeyLog2Clip;
    static const char*		sKeyLog2Scale;
    static const char*		sKeyLog2Style;
    static const char*		sKeyLog2Color;
    static const char*		sKeyLog2FillColor;
    static const char*		sKeyLog2SeisFillColor;
};

} // namespace visSurvey


#endif
