#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.35 2002-05-22 09:03:28 kristofer Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
#include "thread.h"
#include "geompos.h"

class UserIDSet;
class PickSet;
class PickSetGroup;
class Color;
class LineStyle;
class ColorTable;
class CallBack;
class CubeSampling;
class AttribSelSpec;
class AttribSlice;
class IOPar;
class MultiID;

namespace visSurvey
{
class Scene;
class PickSetDisplay;
class WellDisplay;
class PlaneDataDisplay;
};

namespace visBase
{
    class Material;
    class ColorSequence;
};

namespace Threads { class Mutex; };


/*! \brief Service provider for application level - Visutes */

class uiVisPartServer : public uiApplPartServer
{
public:
			uiVisPartServer(uiApplService&);
			~uiVisPartServer();
    const char*		name() const		{ return "Visualisation"; }

    			//Events and their functions
    static const int	evManipulatorMove;
    static const int	evSelection;
    static const int	evDeSelection;
    static const int	evPicksChanged;
    static const int	evGetNewData;
    static const int	evSelectableStatusCh;
    static const int	evMouseMove;
    int			getEventObjId() const { return eventobjid; }
    			/* Tells which object the event is about */

    			//General stuff
    bool		deleteAllObjects();

    void		usePar( const IOPar& );
    void		fillPar( IOPar& ) const;

    enum ElementType    { Inline, Crossline, Timeslice };
    enum ObjectType	{ Unknown, DataDisplay, PickSetDisplay, WellDisplay };
    ObjectType		getObjectType( int ) const;
    void		setObjectName(int,const char*);
    const char*		getObjectName(int);

    			/* Mouse stuff */
    Geometry::Pos	getMousePos(bool xyt) const
    			{ return xyt ? xytmousepos : inlcrlmousepos; }
    			/*!< If !xyt mouse pos will be in inl, crl, t */
    float		getMousePosVal() const { return mouseposval; }

    void		turnOn(int,bool);
    bool		isOn(int);
    void		setViewMode(bool yn);
    void		showAnnotText(int,bool);
    bool		isAnnotTextShown(int);
    void		showAnnotScale(int,bool);
    bool		isAnnotScaleShown(int);
    void		showAnnot(int,bool);
    bool		isAnnotShown(int);

    			// Selection
    bool		isSelectable(int) const;
    void		makeSelectable(int, bool yn );
    void		setSelObjectId(int);
    			//!< set to -1 if you want toedeselect
    int			getSelObjectId() const;

    			// Scene Stuff
    int			addScene();
    void		removeScene(int);
    void		setSelSceneId(int id)	{ selsceneid = id; }
    int			getSelSceneId()		{ return selsceneid; }
    void		getSceneIds(TypeSet<int>&);

			//DataDisplay stuff
    int			addDataDisplay(uiVisPartServer::ElementType);
    void		removeDataDisplay(int);
    void		resetManipulation( int );
    void		setPlanePos(int);
    float		getPlanePos(int) const;
    bool		isPlaneManipulated(int) const;
    void		setAttribSelSpec(int,AttribSelSpec&);
    CubeSampling&	getCubeSampling(int,bool manippos);
    AttribSelSpec&	getAttribSelSpec(int);
    void		putNewData(int,AttribSlice*);
    void		getDataDisplayIds(int,uiVisPartServer::ElementType,
					  TypeSet<int>&);

    			//PickSets stuff
    int                 addPickSetDisplay();
    void                removePickSetDisplay(int);
    bool		setPicks(int, const PickSet&);
    void		getAllPickSets(UserIDSet&);
    void		getPickSetData(const char*,PickSet&);
    void		getPickSetIds(int,TypeSet<int>&);
    int 		nrPicks(int);
    bool		picksetIsPresent(const char*);
    void		showAllPicks(int,bool);
    bool		allPicksShown(int);

    			//Well stuff
    int			addWellDisplay(const MultiID& emwellid);
    void		removeWellDisplay(int);
    int			getNrWellAttribs(int) const;
    const char*		getWellAttribName(int, int idx) const;
    void		displayWellAttrib(int, int idx);
    int			displayedWellAttrib(int) const;
    const LineStyle*	wellLineStyle(int) const;
    void		setWellLineStyle(int, const LineStyle& );

			//ColorSeqs
    bool		canSetColorSeq(int) const;
    void		modifyColorSeq(int, const ColorTable&);
    const ColorTable&   getColorSeq(int) const;
    void		shareColorSeq(int toid, int fromid );

    			//Ranges
    bool		canSetRange(int) const;
    void		setClipRate(int,float);
    float		getClipRate(int) const;
    void		setAutoscale(int,bool);
    bool		getAutoscale(int) const;
    void		setDataRange(int,const Interval<float>&);
    Interval<float>	getDataRange(int) const;
    void		shareRange(int toid, int fromid );
    			//!<Includes colorsequence

    			//Material
    bool		canSetColor(int) const;
    void		modifyColor( int, const Color&);
    Color		getColor(int) const;
    void		shareColor(int toid, int fromid );

			//Dialogs
    bool		setZScale();
    void		setMaterial(int);
    void		setPickSize(int);

protected:

    void		selectObjCB(CallBacker*);
    void		deselectObjCB(CallBacker*);
    void		picksChangedCB(CallBacker*);
    void		manipMoveCB(CallBacker*);
    void		getDataCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);

    const CallBack	appcb;
    int			selsceneid;
    bool		viewmode;

    Threads::Mutex&	eventmutex;
    int			eventobjid;

    Geometry::Pos	xytmousepos;
    Geometry::Pos	inlcrlmousepos;
    float		mouseposval;

    ObjectSet<visSurvey::PickSetDisplay>	picks;
    ObjectSet<visSurvey::PlaneDataDisplay>	seisdisps;
    ObjectSet<visSurvey::WellDisplay>         	wells;
    ObjectSet<visSurvey::Scene>         	scenes;

    static const char*	appvelstr;
};

#endif
