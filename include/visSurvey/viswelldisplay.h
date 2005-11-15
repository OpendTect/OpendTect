#ifndef viswelldisplay_h
#define viswelldisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viswelldisplay.h,v 1.27 2005-11-15 16:16:56 cvshelene Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"
#include "visdatagroup.h"
#include "vistransform.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "visevent.h"
#include "multiid.h"
#include "ranges.h"

class LineStyle;
template <class T> class Interval;

namespace visBase { class Well; class Transformation; };

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

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    void 			setDisplayTransformForPicks(mVisTrans*);

    void                        setSceneEventCatcher(visBase::EventCatcher*);
    void 			addPick(const Coord3&);
    void			connectPicks();//TODO
    void			setupPicking();
    NotifierAccess*             getManipulationNotifier() { return &changed_; }
    bool			isHomeMadeWell() const { return picksallowed_; }
    TypeSet<Coord3>             getWellCoords()	const	{ return wellcoords_; }
    				//only used for user-made wells

protected:

    virtual			~WellDisplay();
    void			setWell(visBase::Well*);
    void			updateMarkers(CallBacker*);
    void			fullRedraw(CallBacker*);

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

    TypeSet<Coord3>		wellcoords_;

    bool			picksallowed_;

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
