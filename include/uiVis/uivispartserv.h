#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.12 2002-04-16 06:48:27 kristofer Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
#include "thread.h"

class UserIDSet;
class PickSet;
class PickSetGroup;
class Color;
class ColorTable;
class CallBack;
class CubeSampling;
class AttribSelSpec;
class AttribSlice;

namespace visSurvey
{
class Scene;
class PickSetDisplay;
class SeisDisplay;
};

namespace visBase{ class Material; };

namespace Threads { class Mutex; };


/*! \brief Service provider for application level - Visutes */

class uiVisPartServer : public uiApplPartServer
{
public:
			uiVisPartServer(uiApplService&);
			~uiVisPartServer();
    const char*		name() const		{ return "Visualisation"; }

    			//Events and their functions
    static const int	evShowPosition;
    static const int	evSelection;
    static const int	evDeSelection;
    static const int	evPicksChanged;
    static const int	evGetNewData;
    int			getEventObjId() const { return eventobjid; }
    			/* Tells which object the event is about */

    			//General stuff
    bool		deleteAllObjects();
    void		setSelObjectId(int);
    			//!< set to -1 if you want to deselect
    int			getSelObjectId() const;

    enum ElementType    { Inline, Crossline, Timeslice };
    enum ObjectType	{ Unknown, DataDisplay, PickSetDisplay };
    ObjectType		getObjectType( int ) const;
    void		setObjectName(int,const char*);
    const char*		getObjectName(int);

    void		turnOn(int,bool);
    bool		isOn(int);
    void		setViewMode(bool yn);

    			// Scene Stuff
    int			addScene();
    visSurvey::Scene*	getScene(int); //Should be removed ->crap!!
    void		setSelSceneId(int id)	{ selsceneid = id; }
    int			getSelSceneId()		{ return selsceneid; }

			//DataDisplay stuff
    int			addDataDisplay(uiVisPartServer::ElementType);
    void		removeDataDisplay(int);
    void		resetManipulation( int );
    float		getPlanePos(int) const;
    void		setAttribSelSpec(int,AttribSelSpec&);
    CubeSampling&	getCubeSampling(int,bool manippos);
    AttribSelSpec&	getAttribSelSpec(int);
    void		putNewData(int,AttribSlice*);

    			//PickSets stuff
    int                 addPickSetDisplay();
    void                removePickSetDisplay(int);
    bool		setPicks(int, const PickSet&);
    void		getAllPickSets(UserIDSet&);
    void		getPickSetData(const char*,PickSet&);
    int 		nrPicks(int);

			//ColorSeqs
    int			setColorSeq(int, ColorTable&);
    const ColorTable&   getColorSeq(int);

    			//Property stuff
    bool		canSetColorSeq(int) const;
    void		linkColorSeq(int fromid, int toid);

    bool		canSetColor(int) const;
    Color		getColor(int) const;
    void		setColor(int,const Color&);
    void		linkColor(int fromid, int toid );
    visBase::Material*	getMaterial(int id);	//Should be removed ASAP

    bool		canSetScale(int);
    void		setClipRate(int,float);
    float		getClipRate(int);
    void		setAutoscale(int,bool);
    bool		getAutoscale(int);
    void		setDataRange(int,const Interval<float>&);
    Interval<float>	getDataRange(int);

protected:

    void		selectObjCB(CallBacker*);
    void		deselectObjCB(CallBacker*);
    void		picksChangedCB(CallBacker*);
    void		showPosCB(CallBacker*);
    void		getDataCB(CallBacker*);

    const CallBack	appcb;
    int			selsceneid;
    bool		viewmode;

    Threads::Mutex&	eventmutex;
    int			eventobjid;

    ObjectSet<visSurvey::PickSetDisplay>	picks;
    ObjectSet<visSurvey::SeisDisplay>		seisdisps;
    ObjectSet<visSurvey::Scene>         	scenes;
};

#endif
